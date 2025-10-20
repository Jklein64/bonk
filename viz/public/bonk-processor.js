class Block {
  /**
   * @param {number} start
   * @param {Float64Array} data
   */
  constructor(start, data) {
    /** @type {Float64Array} */
    this.data = data;

    /** @type {number} */
    this.start = start;
  }

  get end() {
    return this.start + this.data.length;
  }
}

class RingBuffer {
  constructor(maxBlocks) {
    this.head = 0;
    this.tail = 0;
    this.count = 0;
    this.isFull = false;
    this.capacity = maxBlocks;
    /** @type {Array<Block>} */
    this.blocks = new Array(this.capacity);
  }

  push(block) {
    if (this.isFull) {
      console.warn("Pushing to an over-full ring buffer!");
    }
    this.blocks[this.head] = block;
    this.head = (this.head + 1) % this.capacity;
    this.count++;

    if (this.head == this.tail && this.count > 0) {
      this.isFull = true;
    }
  }

  peek() {
    if (this.count === 0) {
      return null;
    }

    return this.blocks[this.tail];
  }

  pop() {
    if (this.count === 0) {
      return null;
    }

    const idx = this.tail;
    this.tail = (this.tail + 1) % this.capacity;
    this.count--;
    this.isFull = false;
    return this.blocks[idx];
  }

  clear() {
    this.tail = this.head;
    this.count = 0;
    this.isFull = false;
  }
}

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

    // TODO make sure this is big enough or handle fetching somehow
    this.blocks = new RingBuffer(4096);
    this.sampleOffset = 0;

    this.port.start();
    this.port.addEventListener("message", (e) => {
      switch (e.data) {
        case "stop":
          this.shouldStop = true;
          return;
        default:
          if (e.data.sampleIdx === 0) {
            // index of the sample at the start of the current block from the processor's perspective
            this.sampleOffset = globalThis.currentFrame;
            // don't clear until we can guarantee we've got a block to play
            this.blocks.clear();
          }
          const start = this.sampleOffset + e.data.sampleIdx;
          const block = new Block(start, new Float64Array(e.data.buffer));
          this.blocks.push(block);

          return;
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

    // Skip blocks that finish after the current callback start sample
    while (this.blocks.peek()?.end < globalThis.currentFrame) {
      this.blocks.pop();
    }

    // Play a block if there is one
    if (this.blocks.count > 0) {
      for (const output of outputs) {
        for (const channel of output) {
          for (let i = 0; i < channel.length; i++) {
            const block = this.blocks.peek();
            channel[i] = block.data[i];
          }
        }
      }
    }

    // still in use
    return true;
  }
}

registerProcessor("bonk-processor", BonkProcessor);
