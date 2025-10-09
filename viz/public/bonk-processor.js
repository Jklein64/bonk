class BonkProcessor extends AudioWorkletProcessor {
  static get parameterDescriptors() {
    return [
      { name: "physicsSampleRate" },
      { name: "physicsBlockSize" },
      { name: "audioSampleRate" },
      { name: "audioBlockSize" },
      { name: "vizSampleRate" },
      { name: "vizBlockSize" },
      { name: "mass" },
      { name: "stiffness" },
      { name: "damping" },
      { name: "area" },
    ];
  }

  constructor(options) {
    super(options);

    console.log("boop!");
    this.port.start();
    this.blocks = [];
    this.sampleIdx = 0;
    this.shouldStop = false;
    this.port.addEventListener("message", (messageEvent) => {
      if (messageEvent.data === "stop") {
        this.shouldStop = true;
      } else {
        const block = new Float64Array(messageEvent.data);
        this.blocks.push(block);
      }
    });
  }

  process(inputs, outputs, parameters) {
    if (this.shouldStop) {
      return false;
    }

    const audioBlockSize = parameters.audioBlockSize[0];
    if (globalThis.currentFrame % (128 * 128) === 0) {
      console.log(parameters.audioBlockSize);
    }
    // if (audioBlockSize != 128) {
    //   console.error("poop");
    // }
    // if (this.blocks.length > 0) {
    //   this.blocks.splice(0, 1);
    // }
    // for (let i = 0; i < outputs[0][0].length; i++) {
    //   const blockIdx = Math.floor(this.sampleIdx / audioBlockSize);
    //   if (blockIdx >= this.blocks.length) {
    //     this.sampleIdx--;
    //   } else {
    //     const sampleIdxWithinBlock = this.sampleIdx % audioBlockSize;
    //     const sample = this.blocks[blockIdx][sampleIdxWithinBlock];
    //     for (const output of outputs) {
    //       for (const channel of output) {
    //         channel[i] = sample;
    //       }
    //     }
    //   }
    //   this.sampleIdx++;
    // }
    // currentFrame is a bad name for this because it has units of samples
    // const currentSample = globalThis.currentFrame;
    // for (const output of outputs) {
    //   for (const channel of output) {
    //     for (let i = 0; i < channel.length; i++) {
    //       channel[i] = Math.random() * 2 - 1;
    //     }
    //   }
    // }
    // still in use
    return true;
  }
}

registerProcessor("bonk-processor", BonkProcessor);
