import React from "react";

const AxisIndicator: React.FC = () => {
  return (
    <div className="axis-indicator">
      <div className="axis-circle y-axis">Y</div>
      <div className="axis-circle z-axis">Z</div>
      <div className="axis-circle x-axis">X</div>
    </div>
  );
};

export default AxisIndicator;
