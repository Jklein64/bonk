from argparse import ArgumentParser
from base64 import b64decode
from scipy.io import wavfile
import urllib3, sseclient
import numpy as np

if __name__ == "__main__":
    parser = ArgumentParser()
    parser.add_argument("--audio_block_size", type=int, default=128)
    parser.add_argument("--output", "-o", required=True)
    args = parser.parse_args()

    stream_retries = urllib3.util.Retry(connect=10, read=10)
    stream_req = urllib3.request("GET", "http://localhost:3000/api/sim/stream/0", preload_content=False, retries=stream_retries)
    stream_client = sseclient.SSEClient(stream_req)

    params = {
        "physicsSampleRate": 1000000,
        "physicsBlockSize": 512,
        "audioSampleRate": 48000,
        "audioBlockSize": args.audio_block_size,
        "vizSampleRate": 25,
        "vizBlockSize": 1,
        "mass": 0.15,
        "stiffness": 5000,
        "damping": 0.1,
        "area": 1,
    }

    urllib3.request("PUT", "http://localhost:3000/api/sim/config/0", json=params)
    urllib3.request("POST", "http://localhost:3000/api/sim/bonk/0", json={
      "x": 2,
      "v": 0,
    })

    try:
        blocks = []
        for event in stream_client.events():
            if event.event == "audio-block":
                block_bytes = b64decode(event.data)
                block = np.frombuffer(block_bytes, dtype=np.float32)
                blocks.append(block)
    except urllib3.exceptions.ReadTimeoutError:
        pass

    result = np.concatenate(blocks)
    wavfile.write(args.output, rate=params["audioSampleRate"], data=result.astype(np.float32))

    
    