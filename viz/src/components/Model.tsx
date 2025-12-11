import React, { useRef } from "react";
import { useGLTF } from "@react-three/drei";
import * as THREE from "three";

interface ModelProps {
  url: string;
  position?: [number, number, number] | THREE.Vector3;
  scale?: number | [number, number, number] | THREE.Vector3;
  rotation?: [number, number, number] | THREE.Euler;
  [key: string]: any;
}

const Model: React.FC<ModelProps> = ({ url, ...props }) => {
  const group = useRef<THREE.Group>(null!);
  const { scene } = useGLTF(url);

  return (
    <group ref={group} {...props}>
      <primitive object={scene.clone()} />
    </group>
  );
};

//preloads the model
export const preloadModel = (url: string) => {
  useGLTF.preload(url);
};

export default Model;
