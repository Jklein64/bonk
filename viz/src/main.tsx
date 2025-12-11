import * as THREE from "three";
import React, { useRef, useState, StrictMode, useMemo, useEffect } from "react";
import { createRoot } from "react-dom/client";
import { Canvas, type ThreeElements } from "@react-three/fiber";
import { OrbitControls } from "@react-three/drei";
import "./index.css";
import "./styles.css";
import { useStream } from "./hooks/useStream";
import { UuidContext } from "./context/uuid";
import useUuid from "./hooks/useUuid";
import AxisIndicator from "./components/AxisIndicator";
import TopBar from "./components/TopBar";
import ControlPanel from "./components/ControlPanel";
import BottomPanel from "./components/BottomPanel";
import Model from "./components/Model";

function Wall(props: ThreeElements["mesh"]) {
  return (
    <mesh {...props}>
      <boxGeometry args={[0.5, 3, 3]} computeBoundingBox={true} />
      <meshStandardMaterial color={getColor("english-violet")} />
    </mesh>
  );
}

type TrianglePrismProps = ThreeElements["mesh"] & {
  onSpringRelease?: (x: number) => void;
  state: { x: number; v: number };
  setState: ({ x, v }: { x: number; v: number }) => void;
};

const TrianglePrism: React.FC<TrianglePrismProps> = ({ onSpringRelease, state, setState, ...props }) => {
  const ref = useRef<THREE.Mesh>(null!);
  const [hover, setHover] = useState(false);
  const [clicking, setClicking] = useState(false);

  const geometry = useMemo(() => {
    let geometry = new THREE.BufferGeometry();

    const vectors = [
      new THREE.Vector3(0, 0, 0),
      new THREE.Vector3(0, 0, 1),
      new THREE.Vector3(0, Math.sqrt(3) / 2, 0.5),
      new THREE.Vector3(0.5, 0, 0),
      new THREE.Vector3(0.5, 0, 1),
      new THREE.Vector3(0.5, Math.sqrt(3) / 2, 0.5),
      // Move so that the object's center is at the local origin
    ].map((v) => v.add(new THREE.Vector3(-0.25, -Math.sqrt(3) / 6, -0.5)));
    const vertices = new Float32Array(vectors.flatMap((v) => v.toArray()));

    const indices = [
      ...[0, 1, 2],
      ...[0, 2, 3],
      ...[3, 2, 5],
      ...[1, 4, 2],
      ...[2, 4, 5],
      ...[0, 4, 1],
      ...[0, 3, 4],
      ...[3, 5, 4],
    ];

    geometry.setIndex(indices);
    geometry.setAttribute("position", new THREE.BufferAttribute(vertices, 3));
    // Needed for flat shading
    geometry = geometry.toNonIndexed();
    geometry.computeVertexNormals();
    return geometry;
  }, []);

  let color = getColor("tiffany-blue");
  if (hover) color = getColor("lavender-pink");
  if (clicking) color = getColor("sky-magenta");

  function onClickDone() {
    setClicking(false);
    const displacement = state.x;
    if (Math.abs(displacement) > 1e-8) {
      onSpringRelease?.(displacement);
    }
  }

  return (
    <mesh
      ref={ref}
      // Hard-coded rest position of 1
      position-x={1 + state.x}
      geometry={geometry}
      onPointerOver={() => setHover(true)}
      onPointerLeave={() => {
        setHover(false);
        if (clicking) {
          onClickDone();
        }
      }}
      onPointerDown={() => {
        setClicking(true);
      }}
      onPointerUp={onClickDone}
      onPointerMove={(e) => {
        if (clicking) {
          // Hard-coded for now
          if (e.point.x >= -0.375) {
            // Hard-coded rest position of 1
            setState({ ...state, x: e.point.x - 1 });
          }
        }
      }}
      scale={clicking ? 1.1 : 1.0}
      {...props}
    >
      <meshStandardMaterial color={color} />
    </mesh>
  );
};

interface AppProps {
  audioContext: React.RefObject<AudioContext>;
  bonkWorkletNode: React.RefObject<AudioWorkletProcessor | null>;
}
const App: React.FC<AppProps> = ({ bonkWorkletNode }) => {
  const { setHandler } = useStream();
  const clientId = useUuid();
  const [state, setState] = useState({
    x: 1,
    v: 0,
  });
  const [params, setParams] = useState({
    physicsSampleRate: 1000000,
    physicsBlockSize: 512,
    audioSampleRate: 48000,
    audioBlockSize: 1024,
    vizSampleRate: 25,
    vizBlockSize: 1,
    // These sound okay...
    mass: 0.15,
    stiffness: 2000,
    damping: 0.1,
    area: 1,
  });

  setHandler("audio-block", (e) => {
    if (!bonkWorkletNode.current) return;
    const buffer = new SharedArrayBuffer(params.audioBlockSize * 8);
    const decodedData = atob(e.data);
    for (let i = 0; i < decodedData.length; i++) {
      new DataView(buffer).setUint8(i, decodedData[i].charCodeAt(0));
    }
    const start = parseInt(e.lastEventId);
    const message = { event: "buffer", buffer, start };
    bonkWorkletNode.current.port.postMessage(message);
  });

  setHandler("viz-block", (e) => {
    const bytes = Uint8Array.from(atob(e.data), (c) => c.charCodeAt(0));
    const values = new Float64Array(bytes.buffer);
    setState({ ...state, x: values[0] });
  });

  const onSpringRelease = (x: number) => {
    const initialState = {
      x,
      v: 0,
    };

    bonkWorkletNode.current?.port.postMessage({ event: "params", params });

    fetch(`/api/sim/config/${clientId}`, {
      method: "PUT",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify(params),
    })
      .catch((err) => console.error(err))
      .then(() => {
        fetch(`/api/sim/bonk/${clientId}`, {
          method: "POST",
          headers: { "Content-Type": "application/json" },
          body: JSON.stringify(initialState),
        }).catch((err) => console.error(err));
      });
  };

  const handleLoadMesh = () => {
    console.log("Load mesh clicked");
  };

  const handleResetSim = () => {
    console.log("Reset sim clicked");
    setState({ x: 1, v: 0 });
  };

  const handleApplyChanges = () => {
    console.log("Apply changes clicked");
  };

  return (
    <div className="app-container">
      <TopBar
        onLoadMesh={handleLoadMesh}
        onResetSim={handleResetSim}
        onApplyChanges={handleApplyChanges}
      />
      <div className="main-content">
        <ControlPanel params={params} setParams={setParams} />
        <div className="canvas-container">
          <Canvas camera={{ position: [3, 3, 3], fov: 50 }}>
            <ambientLight intensity={Math.PI / 2} />
            <spotLight position={[10, 20, 20]} angle={0.15} penumbra={1} decay={0.125} intensity={Math.PI} />
            <pointLight position={[-10, -10, -10]} decay={0.25} intensity={Math.PI} />
            <Wall position={[-1, 0, 0]} />
            <TrianglePrism position={[1, 0, 0]} state={state} setState={setState} onSpringRelease={onSpringRelease} />
            {/* Add your Blender model here - uncomment and update the path */}
            <Model url="/models/HORNET.glb" position={[0, 0, 0]} scale={1} />
            <OrbitControls enableDamping dampingFactor={0.05} />
          </Canvas>
          <AxisIndicator />
        </div>
      </div>
      <BottomPanel />
    </div>
  );
};

interface AudioContextProviderProps {
  children: (params: {
    audioContext: React.RefObject<AudioContext>;
    bonkWorkletNode: React.RefObject<AudioWorkletNode | null>;
  }) => React.ReactElement;
}
const AudioContextProvider: React.FC<AudioContextProviderProps> = (props) => {
  // TODO adjust the sample rate based on the parameters
  const audioContext = useRef(new AudioContext({ sampleRate: 48000 }));
  const bonkWorkletNode = useRef<AudioWorkletNode>(null);

  useEffect(() => {
    audioContext.current.audioWorklet.addModule("bonk-processor.js").then(() => {
      if (bonkWorkletNode.current) {
        // Disconnect old node before replacing. Have to signal to get process() to return false
        // See https://developer.mozilla.org/en-US/docs/Web/API/AudioWorkletProcessor/process#return_value
        bonkWorkletNode.current.port.postMessage("stop");
        bonkWorkletNode.current.disconnect();
        bonkWorkletNode.current.port.close();
      }
      bonkWorkletNode.current = new AudioWorkletNode(audioContext.current, "bonk-processor");
      bonkWorkletNode.current.connect(audioContext.current.destination);
      bonkWorkletNode.current.port.start();
    });

    window.addEventListener("click", () => {
      if (audioContext.current.state === "suspended") {
        audioContext.current.resume();
      }
    });
  }, []);

  return props.children({ audioContext, bonkWorkletNode });
};

createRoot(document.getElementById("root")!).render(
  <StrictMode>
    <UuidContext.Provider value={crypto.randomUUID()}>
      <AudioContextProvider>{(params) => <App {...params} />}</AudioContextProvider>
    </UuidContext.Provider>
  </StrictMode>
);

function getColor(name: string) {
  const value = getComputedStyle(document.documentElement).getPropertyValue(`--color-${name}`);
  if (value === "") console.warn(`No color found with name ${name}.`);
  return value;
}
