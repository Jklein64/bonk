import React, { useState } from "react";

interface ControlPanelProps {
  params: {
    physicsSampleRate: number;
    physicsBlockSize: number;
    audioSampleRate: number;
    audioBlockSize: number;
    vizSampleRate: number;
    vizBlockSize: number;
    mass: number;
    stiffness: number;
    damping: number;
    area: number;
  };
  setParams: React.Dispatch<React.SetStateAction<ControlPanelProps["params"]>>;
}

const ControlPanel: React.FC<ControlPanelProps> = ({ params, setParams }) => {
  const [editingField, setEditingField] = useState<string | null>(null);
  const [editValue, setEditValue] = useState("");

  const handleValueClick = (field: string, currentValue: number) => {
    setEditingField(field);
    setEditValue(String(currentValue));
  };

  const handleValueChange = (e: React.ChangeEvent<HTMLInputElement>) => {
    setEditValue(e.target.value);
  };

  const handleValueBlur = (field: keyof ControlPanelProps["params"]) => {
    const numValue = parseFloat(editValue);
    if (!isNaN(numValue)) {
      setParams((prev) => ({ ...prev, [field]: numValue }));
    }
    setEditingField(null);
  };

  const handleKeyDown = (e: React.KeyboardEvent<HTMLInputElement>, field: keyof ControlPanelProps["params"]) => {
    if (e.key === "Enter") {
      handleValueBlur(field);
    } else if (e.key === "Escape") {
      setEditingField(null);
    }
  };

  const renderEditableValue = (field: keyof ControlPanelProps["params"], value: number) => {
    if (editingField === field) {
      return (
        <input
          type="text"
          className="param-value-input"
          value={editValue}
          onChange={handleValueChange}
          onBlur={() => handleValueBlur(field)}
          onKeyDown={(e) => handleKeyDown(e, field)}
          autoFocus
        />
      );
    }
    return (
      <a href="#" className="param-value-link" onClick={(e) => { e.preventDefault(); handleValueClick(field, value); }}>
        {value}
      </a>
    );
  };
  return (
    <div className="control-panel">
      <div className="control-section">
        <h3 className="control-section-title">Bonk Profile</h3>
        <div className="param-row">
          <div className="param-item">
            <div className="param-label">duration (s)</div>
            <a href="#" className="param-value-link">0.00001241</a>
          </div>
          <div className="param-item">
            <div className="param-label">maximum (N)</div>
            <a href="#" className="param-value-link">12.41</a>
          </div>
          <div className="param-item">
            <div className="param-label">smoothness (?)</div>
            <a href="#" className="param-value-link">20</a>
          </div>
        </div>
        <div className="bonk-profile-graph">
          {/* Placeholder for bonk profile waveform */}
          <svg width="100%" height="80" style={{ background: "transparent" }}>
            <polyline
              points="5,60 20,40 40,30 60,25 80,30 100,35 120,45 140,55 160,60 180,62 200,63 220,64 240,64 260,64"
              fill="none"
              stroke="#e91e63"
              strokeWidth="2"
            />
          </svg>
        </div>
      </div>

      <div className="control-section">
        <h3 className="control-section-title">Parameters</h3>
        <div className="param-row">
          <div className="param-item">
            <div className="param-label">stiffness (N/m)</div>
            {renderEditableValue("stiffness", params.stiffness)}
          </div>
          <div className="param-item">
            <div className="param-label">mass (kg)</div>
            {renderEditableValue("mass", params.mass)}
          </div>
          <div className="param-item">
            <div className="param-label">damping</div>
            {renderEditableValue("damping", params.damping)}
          </div>
        </div>
        <div className="param-row">
          <div className="param-item">
            <div className="param-label">audio block size</div>
            {renderEditableValue("audioBlockSize", params.audioBlockSize)}
          </div>
          <div className="param-item">
            <div className="param-label">audio sample rate (Hz)</div>
            {renderEditableValue("audioSampleRate", params.audioSampleRate)}
          </div>
        </div>
        <div className="param-row">
          <div className="param-item">
            <div className="param-label">display block size</div>
            {renderEditableValue("vizBlockSize", params.vizBlockSize)}
          </div>
          <div className="param-item">
            <div className="param-label">display sample rate (Hz)</div>
            {renderEditableValue("vizSampleRate", params.vizSampleRate)}
          </div>
        </div>
      </div>

      <div className="control-section">
        <h3 className="control-section-title">Microphone</h3>
        <div className="param-row">
          <div className="param-item">
            <div className="param-label">position</div>
            <div className="param-value-group">
              <a href="#" className="param-value-link">20.2</a>
              <a href="#" className="param-value-link">20.2</a>
              <a href="#" className="param-value-link">20.2</a>
            </div>
          </div>
          <div className="param-item">
            <div className="param-label">look at</div>
            <div className="param-value-group">
              <a href="#" className="param-value-link">0</a>
              <a href="#" className="param-value-link">0</a>
              <a href="#" className="param-value-link">0</a>
            </div>
          </div>
        </div>
      </div>

      <div className="control-section">
        <h3 className="control-section-title">Logs</h3>
        <div className="logs-container">
          <div className="log-entry log-info">[info] established connection to server</div>
          <div className="log-entry log-info">[info] received mesh. uploading ...</div>
          <div className="log-entry log-error">[error] tetrahedralization failed! mesh was not watertight</div>
        </div>
      </div>
    </div>
  );
};

export default ControlPanel;
