import { useEffect, useRef, type RefObject } from "react";
import useUuid from "../hooks/useUuid";

// work with base64 strings for now
export type DataCallback = (data: string) => void;

export function useStream(name: string) {
  const clientId = useUuid();
  const streamUrl = `/api/stream/${name}/${clientId}`;
  const source = useRef<EventSource>(null!);
  
  const subscribers: RefObject<Set<DataCallback>> = useRef(new Set());
  
  useEffect(() => {
    console.log("creating event source with url " + streamUrl);
    const es = new EventSource(streamUrl)
    source.current = es

    es.onopen = (ev) => {
      console.log("opened stream!");
      console.log(ev);
    };

    es.onmessage = (ev) => {
      for (const callback of subscribers.current) {
        callback(ev.data);
      }
    };

    return () => {
      console.log("closing stream!")
      source.current.close()
    }
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
