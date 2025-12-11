import { createContext } from "react";

export const UuidContext = createContext<`${string}-${string}-${string}-${string}-${string}` | null>(null);