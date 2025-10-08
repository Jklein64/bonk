import { useEffect, useRef, type RefObject } from "react";

// work with base64 strings for now
export type DataCallback = (data: string) => void;

export function useStream(name: string) {
  const streamUrl = `/api/stream/${name}`;
  console.log("creating event source with url " + streamUrl);
  const source = useRef(new EventSource(streamUrl));

  const subscribers: RefObject<Set<DataCallback>> = useRef(new Set());

  useEffect(() => {
    source.current.onopen = (ev) => {
      console.log("opened stream!");
      console.log(ev);
    };

    source.current.onmessage = (ev) => {
      for (const callback of subscribers.current) {
        callback(ev.data);
      }
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
