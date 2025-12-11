import React, { useState } from "react";

const BottomPanel: React.FC = () => {
  const [reverbPreset, setReverbPreset] = useState<string>("(no reverb)");
  const [dryWet, setDryWet] = useState<number>(50);

  const presets = [
    "(no reverb)",
    "Small Room",
    "Medium Hall",
    "Large Cathedral",
    "Plate",
    "Spring",
  ];

  return (
    <div className="bottom-panel">
      <div className="bottom-section reverb-section">
        <h3 className="bottom-section-title">Reverb</h3>
        <div className="reverb-controls">
          <div className="reverb-presets">
            <div className="param-label">preset</div>
            <select
              className="reverb-preset-select"
              value={reverbPreset}
              onChange={(e) => setReverbPreset(e.target.value)}
            >
              {presets.map((preset) => (
                <option key={preset} value={preset}>
                  {preset}
                </option>
              ))}
            </select>
          </div>
          <div className="reverb-drywet">
            <div className="param-label">dry/wet (%)</div>
            <div className="drywet-slider-container">
              <input
                type="range"
                min="0"
                max="100"
                value={dryWet}
                onChange={(e) => setDryWet(parseInt(e.target.value))}
                className="drywet-slider"
              />
              <span className="drywet-value">{dryWet}</span>
            </div>
          </div>
        </div>
      </div>
      <div className="bottom-section scope-section">
        <h3 className="bottom-section-title">Scope</h3>
        <div className="scope-container">
          {/* Placeholder for audio scope/waveform visualization */}
          <svg width="100%" height="100" style={{ background: "transparent" }}>
            <polyline
              points="0,50 50,30 100,60 150,25 200,70 250,35 300,55 350,40 400,50 450,45 500,50 550,55 600,50"
              fill="none"
              stroke="#4a9eff"
              strokeWidth="1.5"
            />
            <line x1="0" y1="50" x2="600" y2="50" stroke="#333" strokeWidth="1" strokeDasharray="5,5" />
          </svg>
        </div>
      </div>
    </div>
  );
};

export default BottomPanel;
