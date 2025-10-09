import { useEffect, useRef, type RefObject } from "react";
import useUuid from "../hooks/useUuid";

export type DataCallback = (data: ArrayBuffer) => void;

export function useStream(name: string) {
  const clientId = useUuid();
  const streamUrl = `/api/stream/${name}/${clientId}`;
  const source = useRef<EventSource>(null!);

  const subscribers: RefObject<Set<DataCallback>> = useRef(new Set());

  useEffect(() => {
    console.log("creating event source with url " + streamUrl);
    const es = new EventSource(streamUrl);
    source.current = es;

    es.onmessage = (ev) => {
      const buffer = Uint8Array.from(atob(ev.data), (c) => c.charCodeAt(0)).buffer;
      for (const callback of subscribers.current) {
        callback(buffer);
      }
    };

    return () => {
      source.current.close();
    };
  }, []);

  function subscribe(callback: DataCallback) {
    subscribers.current.add(callback);
  }

  function unsubscribe(callback: DataCallback) {
    const unsubscribed = subscribers.current.delete(callback);
    if (!unsubscribed) console.warn("Attempted to unsubscribe a callback that was never registered.");
  }

  return {
    subscribe,
    unsubscribe,
  };
}
