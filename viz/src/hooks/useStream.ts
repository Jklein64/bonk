import { useEffect, useRef } from "react";
import useUuid from "../hooks/useUuid";

type EventType = "audio-block" | "heartbeat";
type CallbackType = (event: any) => void;

export function useStream() {
  const clientId = useUuid();
  const streamUrl = `/api/sim/stream/${clientId}`;
  const source = useRef<EventSource>(null!);
  const handlers = useRef<Map<EventType, CallbackType>>(new Map());

  const setHandler = (name: EventType, handler: CallbackType) => {
    handlers.current.set(name, handler);
  };

  useEffect(() => {
    console.log("creating event source with url " + streamUrl);
    const es = new EventSource(streamUrl);
    source.current = es;

    handlers.current.forEach((handler, name) => {
      source.current.addEventListener(name, (event) => handler(event));
    });

    return () => {
      source.current.close();
    };
  }, []);

  return { setHandler };
}
