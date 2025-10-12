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

    this.blocks = [];
    this.sampleIdx = 0;
    this.shouldStop = false;

    this.port.start();
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
    if (audioBlockSize !== 128) {
      console.error("poop!");
    }

    // Play a block if there is one
    if (this.blocks.length > 0) {
      for (const output of outputs) {
        for (const channel of output) {
          for (let i = 0; i < channel.length; i++) {
            channel[i] = this.blocks[0][i];
          }
        }
      }

      this.blocks.splice(0, 1);
    }

    // still in use
    return true;
  }
}

registerProcessor("bonk-processor", BonkProcessor);
