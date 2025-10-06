import * as THREE from "three";
import { useRef, useState, StrictMode, useMemo } from "react";
import { createRoot } from "react-dom/client";
import { Canvas, useFrame, type ThreeElements } from "@react-three/fiber";
import "./index.css";

function Box(props: ThreeElements["mesh"]) {
  const ref = useRef<THREE.Mesh>(null!);
  const [hovered, hover] = useState(false);
  const [clicked, click] = useState(false);
  useFrame((state, delta) => (ref.current.rotation.x += delta));
  return (
    <mesh
      {...props}
      ref={ref}
      scale={clicked ? 1.5 : 1}
      onClick={(event) => click(!clicked)}
      onPointerOver={(event) => hover(true)}
      onPointerOut={(event) => hover(false)}
    >
      <boxGeometry args={[1, 1, 1]} />
      <meshStandardMaterial color={hovered ? getColor("sky-magenta") : getColor("tiffany-blue")} />
    </mesh>
  );
}

function Wall(props: ThreeElements["mesh"]) {
  return (
    <mesh {...props}>
      <boxGeometry args={[0.5, 3, 3]} />
      <meshStandardMaterial color={getColor("english-violet")} />
    </mesh>
  );
}

function TrianglePrism(props: ThreeElements["mesh"]) {
  const ref = useRef<THREE.Mesh>(null!);
  const [hover, setHover] = useState(false);
  const [clicking, setClicking] = useState(false);
  const [preDragPoint, setPreDragPoint] = useState<THREE.Vector3 | undefined>();
  const [displacement, setDisplacement] = useState(0);

  const geometry = useMemo(() => {
    let geometry = new THREE.BufferGeometry();

    const vectors = [
      new THREE.Vector3(0, 0, 0),
      new THREE.Vector3(0, 0, 1),
      new THREE.Vector3(0, Math.sqrt(3) / 2, 0.5),
      new THREE.Vector3(0.5, 0, 0),
      new THREE.Vector3(0.5, 0, 1),
      new THREE.Vector3(0.5, Math.sqrt(3) / 2, 0.5),
    ].map((v) => v.add(new THREE.Vector3(-0.25, -Math.sqrt(3) / 6, -0.5)));

    const vertices = new Float32Array([
      ...vectors[0].toArray(), // v0
      ...vectors[1].toArray(), // v1
      ...vectors[2].toArray(), // v2
      ...vectors[3].toArray(), // v3
      ...vectors[4].toArray(), // v4
      ...vectors[5].toArray(), // v5
    ]);

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

  return (
    <mesh
      ref={ref}
      geometry={geometry}
      onPointerOver={() => setHover(true)}
      onPointerLeave={() => {
        setHover(false);
        // setClicking(false);
        setPreDragPoint(undefined);
      }}
      onPointerDown={(e) => {
        setClicking(true);
        setPreDragPoint(e.point);
      }}
      onPointerUp={() => {
        setClicking(false);
        setPreDragPoint(undefined);
      }}
      onPointerMove={(e) => {
        if (clicking && preDragPoint) {
          const d = e.point.sub(preDragPoint).x;
          setDisplacement(d);
          console.log(displacement);
          ref.current.position.setX(preDragPoint.x + d);
        }
      }}
      scale={clicking ? 1.1 : 1.0}
      {...props}
    >
      <meshStandardMaterial color={color} />
    </mesh>
  );
}

createRoot(document.getElementById("root")!).render(
  <StrictMode>
    <Canvas>
      <ambientLight intensity={Math.PI / 2} />
      <spotLight position={[10, 20, 20]} angle={0.15} penumbra={1} decay={0.125} intensity={Math.PI} />
      <pointLight position={[-10, -10, -10]} decay={0.25} intensity={Math.PI} />
      {/* <directionalLight position={[1, 1, 1]} intensity={Math.PI} /> */}
      {/* <Box position={[-1.2, 0, 0]} />
    <Box position={[1.2, 0, 0]} /> */}
      <Wall position={[-1, 0, 0]} />
      <TrianglePrism position={[1, 0, 0]} />
    </Canvas>
  </StrictMode>
);

function getColor(name: string) {
  const value = getComputedStyle(document.documentElement).getPropertyValue(`--color-${name}`);
  if (value === "") console.warn(`No color found with name ${name}.`);
  return value;
}
