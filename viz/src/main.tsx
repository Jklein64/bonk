import * as THREE from "three";
import { useRef, useState, StrictMode, useMemo, useEffect } from "react";
import { createRoot } from "react-dom/client";
import { Canvas, type ThreeElements } from "@react-three/fiber";
import "./index.css";
import { useStream, type DataCallback } from "./hooks/useStream";

function Wall(props: ThreeElements["mesh"]) {
  return (
    <mesh {...props}>
      <boxGeometry args={[0.5, 3, 3]} computeBoundingBox={true} />
      <meshStandardMaterial color={getColor("english-violet")} />
    </mesh>
  );
}

function TrianglePrism(props: ThreeElements["mesh"]) {
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
    // Hard-coded rest position of 1
    const displacement = ref.current.position.x - 1;
    if (Math.abs(displacement) > 1e-8) {
      configureSim(displacement);
    }
  }

  return (
    <mesh
      ref={ref}
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
            ref.current.position.setX(e.point.x);
          }
        }
      }}
      scale={clicking ? 1.1 : 1.0}
      {...props}
    >
      <meshStandardMaterial color={color} />
    </mesh>
  );
}

function App({ children }: { children: any }) {
  const audioContext = useRef(new AudioContext());
  // const { subscribe, unsubscribe } = useStream("audio");

  useEffect(() => {
    audioContext.current.audioWorklet.addModule("bonk-processor.js").then(() => {
      const bonkWorkletNode = new AudioWorkletNode(audioContext.current, "bonk-processor");
      bonkWorkletNode.connect(audioContext.current.destination);
    });

    // See https://developer.chrome.com/blog/autoplay/#web_audio
    // window.addEventListener("click", () => {
    //   if (audioContext.current.state === "suspended") {
    //     audioContext.current.resume();
    //   }
    // });
  }, []);

  // useEffect(() => {
  //   const callback: DataCallback = (data) => {
  //     console.log("received data from the audio stream!");
  //     console.log(data);
  //   };

  //   subscribe(callback);

  //   return () => {
  //     unsubscribe(callback);
  //   };
  // });

  return <>{children}</>;
}

createRoot(document.getElementById("root")!).render(
  <StrictMode>
    <App>
      <Canvas>
        <ambientLight intensity={Math.PI / 2} />
        <spotLight position={[10, 20, 20]} angle={0.15} penumbra={1} decay={0.125} intensity={Math.PI} />
        <pointLight position={[-10, -10, -10]} decay={0.25} intensity={Math.PI} />
        <Wall position={[-1, 0, 0]} />
        <TrianglePrism position={[1, 0, 0]} />
      </Canvas>
    </App>
  </StrictMode>
);

function getColor(name: string) {
  const value = getComputedStyle(document.documentElement).getPropertyValue(`--color-${name}`);
  if (value === "") console.warn(`No color found with name ${name}.`);
  return value;
}

function configureSim(displacement: number) {
  const params = {
    physicsSampleRate: 1000000,
    physicsBlockSize: 512,
    audioSampleRate: 48000,
    audioBlockSizessss: 128,
    vizSampleRate: 30,
    vizBlockSize: 5,
    // These sound okay...
    mass: 0.005,
    stiffness: 3000,
    damping: 0.12,
    area: 1,
  };
  const initialState = {
    x: displacement,
    v: 0,
  };

  console.log({ params, initialState });
  fetch("/api/configure", {
    method: "POST",
    headers: { "Content-Type": "application/json" },
    body: JSON.stringify({ params, initialState }),
  }).catch((err) => console.error(err));
}
