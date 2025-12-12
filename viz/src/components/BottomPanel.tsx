import React, { useState } from "react";

interface BottomPanelProps {
  bonkProfile?: number[];
  audioScope?: number[];
}

const BottomPanel: React.FC<BottomPanelProps> = ({ bonkProfile = [], audioScope = [] }) => {
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

  // Generate polyline points from data
  const generatePolyline = (data: number[], width: number, height: number) => {
    if (data.length === 0) return "";
    const step = width / Math.max(data.length - 1, 1);
    const maxVal = Math.max(...data, 0.001);
    const minVal = Math.min(...data, 0);
    const range = maxVal - minVal;

    return data.map((val, i) => {
      const x = i * step;
      const y = height - ((val - minVal) / range) * height * 0.8 - height * 0.1;
      return `${x},${y}`;
    }).join(" ");
  };

  // Calculate stats for bonk profile
  const duration = bonkProfile.length > 0 ? bonkProfile.length / 48000 : 0;
  const maximum = bonkProfile.length > 0 ? Math.max(...bonkProfile.map(Math.abs)) : 0;

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

      <div className="bottom-section bonk-profile-section">
        <h3 className="bottom-section-title">Bonk Profile</h3>
        <div className="profile-stats">
          <div className="stat">
            <div className="stat-label">duration (s)</div>
            <div className="stat-value">{duration.toFixed(8)}</div>
          </div>
          <div className="stat">
            <div className="stat-label">maximum (n)</div>
            <div className="stat-value">{maximum.toFixed(2)}</div>
          </div>
          <div className="stat">
            <div className="stat-label">smoothness (?)</div>
            <div className="stat-value">20</div>
          </div>
        </div>
        <div className="scope-container">
          <svg width="100%" height="100" style={{ background: "transparent" }}>
            {bonkProfile.length > 0 ? (
              <polyline
                points={generatePolyline(bonkProfile, 600, 100)}
                fill="none"
                stroke="#ff4a7e"
                strokeWidth="1.5"
              />
            ) : (
              <text x="50%" y="50%" textAnchor="middle" fill="#666" fontSize="12">
                No data - drag and release the spring to generate profile
              </text>
            )}
          </svg>
        </div>
      </div>

      <div className="bottom-section scope-section">
        <h3 className="bottom-section-title">Scope</h3>
        <div className="scope-container">
          <svg width="100%" height="100" style={{ background: "transparent" }}>
            {audioScope.length > 0 ? (
              <>
                <polyline
                  points={generatePolyline(audioScope, 600, 100)}
                  fill="none"
                  stroke="#4a9eff"
                  strokeWidth="1.5"
                />
                <line x1="0" y1="50" x2="600" y2="50" stroke="#333" strokeWidth="1" strokeDasharray="5,5" />
              </>
            ) : (
              <text x="50%" y="50%" textAnchor="middle" fill="#666" fontSize="12">
                No audio data
              </text>
            )}
          </svg>
        </div>
      </div>
    </div>
  );
};

export default BottomPanel;
