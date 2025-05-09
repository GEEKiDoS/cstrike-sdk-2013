import { test_shit } from 'module_test.js';

async function main() {
    console.log('hahaha I\'m working!!!');
    console.log('test shit %s', test_shit);

    console.assert(false, 'assertion test');

    const cvar = engine.findCVar("mp_flashlight");
    if (!cvar.value) {
        console.warn("mp_flashlight isn't on");
        cvar.value = true;

        console.log('now it\'s on');
    }

    console.log("mp_flashlight: %d", cvar.value);

    console.log('inf loop (promise) test');

    let lastOutput = engine.getServerTime();
    let frames = 0;
    while(true)
    {
        await nextFrame();
        ++frames;

        const now = engine.getServerTime();
        if (now - lastOutput > 1.0)
        {
            console.log("looping... fps: %d\n", frames);

            frames = 0;
            lastOutput = now;
        }
    }
}

main();
