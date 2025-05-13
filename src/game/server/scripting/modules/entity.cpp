#include "cbase.h"

#undef str_size
#include "../qjs/quickjs.h"
#include "../qjs/quickjs-libc.h"
#include "../scripting.h"

namespace Entity
{
	namespace EntityObject
	{
		JSClassID classId;

		JSValue New(JSContext* ctx, CBaseEntity* entity)
		{
			auto ent = JS_NewObjectClass(ctx, classId);
			JS_SetOpaque(ent, entity);

			return ent;
		}

		void Finalizer(JSRuntime*, JSValue) {}

		JSClassDef classDef{ "Entity", Finalizer };
		JSCFunctionListEntry methods[] = {
			JS_PROP_INT32_DEF("dummy", 0, JS_PROP_CONFIGURABLE),
		};

		void Init(JSContext* ctx)
		{
			auto* rt = JS_GetRuntime(ctx);

			JS_NewClassID(rt, &classId);
			JS_NewClass(rt, classId, &classDef);

			JSValue proto = JS_NewObject(ctx);
			JS_SetPropertyFunctionList(ctx, proto, methods, _countof(methods));
			JS_SetClassProto(ctx, classId, proto);
		}
	}

	namespace EntityIterator
	{
		enum FindType: int32_t
		{
			INVALID,
			FIND_BY_CLASSNAME,
			FIND_BY_MODEL,
			FIND_BY_NAME,
			FIND_BY_TARGET,
		};

		JSClassID classId;

		JSValue New(JSContext* ctx, FindType type, JSValue query)
		{
			auto iterator = JS_NewObjectClass(ctx, classId);
			JS_SetOpaque(iterator, nullptr);
			JS_SetPropertyStr(ctx, iterator, "__type", JS_NewInt32(ctx, type));
			JS_SetPropertyStr(ctx, iterator, "__query", JS_DupValue(ctx, query));

			return iterator;
		}

		JSValue next(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv, int* pdone, int magic)
		{
			auto ent = reinterpret_cast<CBaseEntity*>(
				JS_GetOpaque(this_val, classId)
			);

			auto type = JS_GetPropertyStr(ctx, this_val, "__type");
			auto query = JS_GetPropertyStr(ctx, this_val, "__query");

			int32_t type_enum = INVALID;
			JS_ToInt32(ctx, &type_enum, type);

			auto* query_str = JS_ToCString(ctx, query);

			switch (type_enum)
			{
			case FIND_BY_CLASSNAME:
				ent = gEntList.FindEntityByClassname(ent, query_str);
				break;
			case FIND_BY_MODEL:
				ent = gEntList.FindEntityByModel(ent, query_str);
				break;
			case FIND_BY_NAME:
				ent = gEntList.FindEntityByName(ent, query_str);
				break;
			case FIND_BY_TARGET:
				ent = gEntList.FindEntityByTarget(ent, query_str);
				break;
			}

			JS_SetOpaque(this_val, ent);
			*pdone = !ent;

			JS_FreeCString(ctx, query_str);
			JS_FreeValue(ctx, query);
			JS_FreeValue(ctx, type);

			if (*pdone)
				return JS_UNDEFINED;

			return EntityObject::New(ctx, ent);
		}

		JSValue symbol_iterator(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv)
		{
			return JS_DupValue(ctx, this_val);
		}

		void Finalizer(JSRuntime*, JSValue) {}

		JSClassDef classDef{ "EntityIterator", Finalizer };
		JSCFunctionListEntry methods[] = {
			JS_ITERATOR_NEXT_DEF("next", 0, next, 0),
			JS_CFUNC_DEF("[Symbol.iterator]", 0, symbol_iterator),
		};

		void Init(JSContext* ctx)
		{
			auto* rt = JS_GetRuntime(ctx);

			JS_NewClassID(rt, &classId);
			JS_NewClass(rt, classId, &classDef);

			JSValue proto = JS_NewObject(ctx);
			JS_SetPropertyFunctionList(ctx, proto, methods, _countof(methods));
			JS_SetClassProto(ctx, classId, proto);
		}
	}

	static JSValue findByClassName(JSContext* ctx, JSValueConst, int argc, JSValueConst* argv)
	{
		if (argc < 1)
			return JS_ThrowTypeError(ctx, "findByClassName need 1 args");

		if (!JS_IsString(*argv))
			return JS_ThrowTypeError(ctx, "findByClassName arg 0 needs to be string");

		return EntityIterator::New(ctx, EntityIterator::FIND_BY_CLASSNAME, *argv);
	}

	static JSValue findByModel(JSContext* ctx, JSValueConst, int argc, JSValueConst* argv)
	{
		if (argc < 1)
			return JS_ThrowTypeError(ctx, "findByModel need 1 args");

		if (!JS_IsString(*argv))
			return JS_ThrowTypeError(ctx, "findByModel arg 0 needs to be string");

		return EntityIterator::New(ctx, EntityIterator::FIND_BY_MODEL, *argv);
	}

	static JSValue findByName(JSContext* ctx, JSValueConst, int argc, JSValueConst* argv)
	{
		if (argc < 1)
			return JS_ThrowTypeError(ctx, "findByName need 1 args");

		if (!JS_IsString(*argv))
			return JS_ThrowTypeError(ctx, "findByName arg 0 needs to be string");

		return EntityIterator::New(ctx, EntityIterator::FIND_BY_NAME, *argv);
	}

	static JSValue findByTarget(JSContext* ctx, JSValueConst, int argc, JSValueConst* argv)
	{
		if (argc < 1)
			return JS_ThrowTypeError(ctx, "findByTarget need 1 args");

		if (!JS_IsString(*argv))
			return JS_ThrowTypeError(ctx, "findByTarget arg 0 needs to be string");

		return EntityIterator::New(ctx, EntityIterator::FIND_BY_TARGET, *argv);
	}

	static const JSCFunctionListEntry funcs[] = {
		JS_CFUNC_DEF("findByClassName", 1, findByClassName),
		JS_CFUNC_DEF("findByModel", 1, findByModel),
		JS_CFUNC_DEF("findByName", 1, findByName),
		JS_CFUNC_DEF("findByTarget", 1, findByTarget),
	};

	static int init(JSContext* ctx, JSModuleDef* m)
	{
		return JS_SetModuleExportList(ctx, m, funcs, _countof(funcs));
	}

	class Module : public IScriptingModule
	{
	public:
		void Init(JSContext* ctx) const
		{
			auto* m = JS_NewCModule(ctx, "entity", init);
			assert(m);

			JS_AddModuleExportList(ctx, m, funcs, _countof(funcs));

			EntityObject::Init(ctx);
			EntityIterator::Init(ctx);
		}
	};
}

REGISTER_SCRIPTING_MODULE(Entity);
