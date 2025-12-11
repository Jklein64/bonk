import React from "react";

interface TopBarProps {
  onLoadMesh?: () => void;
  onResetSim?: () => void;
  onApplyChanges?: () => void;
}

const TopBar: React.FC<TopBarProps> = ({ onLoadMesh, onResetSim, onApplyChanges }) => {
  return (
    <div className="top-bar">
      <div className="top-bar-branding">
        <img
          src="https://avatars.githubusercontent.com/u/11234567?v=4"
          alt="Profile"
          className="top-bar-avatar"
        />
        <div className="top-bar-text">
          <h1 className="top-bar-title">Bonk</h1>
          <div className="top-bar-subtitle">Cornell CS 4621 (Fall 2025)</div>
        </div>
        <a
          href="http://github.com/jklein64/bonk"
          target="_blank"
          rel="noopener noreferrer"
          className="top-bar-github"
        >
          http://github.com/jklein64/bonk
        </a>
      </div>
      <div className="top-bar-controls">
        <button className="top-bar-button" onClick={onLoadMesh}>load mesh</button>
        <button className="top-bar-button" onClick={onResetSim}>reset sim</button>
        <button className="top-bar-button" onClick={onApplyChanges}>apply changes</button>
      </div>
    </div>
  );
};

export default TopBar;
