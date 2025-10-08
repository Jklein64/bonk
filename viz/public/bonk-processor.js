class BonkProcessor extends AudioWorkletProcessor {
  constructor(options) {
    super(options);
  }

  process(inputs, outputs, parameters) {
    for (const output of outputs) {
      for (const channel of output) {
        for (let i = 0; i < channel.length; i++) {
          channel[i] = Math.random() * 2 - 1;
        }
      }
    }
    // still in use
    return true;
  }
}

registerProcessor("bonk-processor", BonkProcessor);
