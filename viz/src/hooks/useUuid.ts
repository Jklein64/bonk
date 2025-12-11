import { useContext } from "react";
import { UuidContext } from "../context/uuid";

export default function useUuid() {
    const context = useContext(UuidContext)
    if (!context) throw new Error("Cannot call useUuid outside of context provider.")
    return context
}