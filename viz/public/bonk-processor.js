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
      console.warn("Popping from an empty ring buffer!");
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
  constructor(options) {
    super(options);

    // TODO make sure this is big enough or handle fetching somehow
    this.blocks = new RingBuffer(4096);
    this.sampleOffset = 0;

    this.port.start();
    this.port.addEventListener("message", (e) => {
      switch (e.data.event) {
        case "stop":
          this.shouldStop = true;
          return;

        case "params":
          const { params } = e.data;
          console.log("updating params:");
          console.log(params);
          this.params = params;
          return;

        case "buffer":
          const { start, buffer } = e.data;
          if (start === 0) {
            // Audio callback block start sample number
            this.sampleOffset = globalThis.currentFrame;
            // Don't clear until we can guarantee we've got a block to play
            this.blocks.clear();
          }
          const block = new Block(start + this.sampleOffset, new Float32Array(buffer));
          this.blocks.push(block);
          return;
      }
    });
  }

  process(inputs, outputs, parameters) {
    if (this.shouldStop) {
      return false;
    }

    if (!this.params) {
      // don't cancel but don't send audio
      return true;
    }

    let currentStart = globalThis.currentFrame;
    // Skip blocks that finish before the current callback start sample
    while (this.blocks.peek()?.end <= currentStart) {
      this.blocks.pop();
    }

    for (let i = 0; i < outputs[0][0].length; i++) {
      let block = this.blocks.peek();
      if (!block) break;
      // Read samples from partway through a block
      let offset = currentStart - block.start;
      // ...overflowing to the next one if needed
      if (offset + i >= block.data.length) {
        this.blocks.pop();
        block = this.blocks.peek();
        if (!block) break;
        offset = currentStart - block.start;
      }
      outputs[0][0][i] = block.data[offset + i];
    }

    // Still in use
    return true;
  }
}

registerProcessor("bonk-processor", BonkProcessor);
