const express = require('express');
const cors = require('cors');
const bonk = require('./build/Release/Bonk.node');
const app = express();
app.use(express.json());
app.use(cors());
const port = process.env.PORT || 3333;

let bonkInstance
try {
  bonkInstance = new bonk.BonkInstance()
} catch (error) {
  console.error("Failed to create BonkInstance")
  process.exit(1)
}

app.get('/status', (req, res) => {
  res.json({status: 'ok', message: 'Running'})
})

app.post('/load', (req, res) => {
  try {
    const {name} = req.body
    if (!name || typeof name != 'string') {
      return res.status(400).json({error: "Invalid argument (load requires string)"})
    }
    response = bonkInstance.loadMesh(name)
    if (response != 0) {
      return res.status(400).json({error: "Mesh loading failed", message: "" + response})
    }
    res.json({success: true})
  } catch(error) {
    console.error(error)
    res.status(500).json({error: "Failed to load mesh", message: error.message})
  }
})

app.post('/readyThree', (req, res) => {
  try {
    response = bonkInstance.prepareThree()
    if (response != 0) {
      return res.status(400).json({error: "Three preparation failed", message: "" + response})
    }
    res.json({success:true})
  } catch (error) {
    console.error(error)
    res.status(500).json({error: "Three preparation failed", message:error.message})
  }
})

app.get('/indices', (req, res) => {
  try {
    const indices = bonkInstance.getIndices()
    res.json({success:true, data:indices})
  } catch(error) {
    console.error(error)
    res.status(500).json({error: "Failed to fetch indices", message:error.message})
  }
})

app.get('/vertices', (req, res) => {
  try {
    const vertices = bonkInstance.getIndices()
    res.json({success:true, data:vertices})
  } catch(error) {
    console.error(error)
    res.status(500).json({error: "Failed to fetch vertices", message:error.message})
  }
})

app.post('/modal', (req, res) => {
  try {
    const {density, k, dt, damping, freqDamping} = req.body
    if (typeof density != 'number' || typeof k != 'number' || typeof dt != 'number') {
      return res.status(400).json({error: "Invalid request (density, k, dt must be numbers)"})
    }
    let response
    if (damping == undefined) {
      response = bonkInstance.initModalContext(density, k, dt)
    }
    else if (freqDamping == undefined) {
      if (typeof damping != 'number') {
        return res.status(400).json({error: "Invalid request (damping must be a number)"})
      }
      else {
        response = bonkInstance.initModalContext(density, k, dt, damping)
      }
    } else {
      if (!(typeof damping == 'number' && typeof freqDamping == 'number')) {
        return res.status(400).json({error: "Invalid request (damping and freqDamping must be numbers)"})
      }
      response = bonkInstance.initModalContext(density, k, dt, damping, freqDamping)
    }
    if (response != 0) {
      return res.status(400).json({error: "Modal preparation failed", message: "" + response})
    }
    res.json({success:true})
  } catch(error) {
    console.error(error)
    res.status(500).json({error: "Failed to set up modal system", message:error.message})
  }
})

app.post('/bonk', (req, res) => {
  try {
    const {indices, weights, force} = req.body
    if (!Array.isArray(indices) || !Array.isArray(weights) || !Array.isArray(force)) {
      return res.status(400).json({error: "Invalid request (indices, weights, force must be arrays)"})
    }
    const response = bonkInstance.bonk(indices, weights, force)
    if (response != 0) {
      return res.status(400).json({error: "Failed to bonk object", message: "" + response})
    }
    res.json({success:true})
  } 
  catch(error) {
    console.error(error)
    res.status(500).json({error: "Failed to bonk object", message:error.message})
  }
})

app.post('/run', (req, res) => {
  try {
    const {count} = req.body
    if (typeof count != number || count < 0) {
      return res.status(400).json({error:"Invalid request (count must be a positive number"})
    }
    const response = bonkInstance.runModal(count)
    if (response = 0) {
      return res.json({success:true, extinction:false})
    } else if (response == 6) {
      return res.json({success:true, extinction:true})
    } else {
      return res.status(400).json({error: "Failed to run modal steps", message: "" + response})
    }
  }
  catch (error) {
    res.status(500).json({error: "Failed to run modal steps", message:error.message})
  }
})

app.get('/results', (req, res) => {
  try {
    const results = bonkInstance.getModalResults()
    res.json({success:true, data:results})
  } catch(error) {
    console.error(error)
    res.status(500).json({error: "Failed to fetch modal results", message:error.message})
  }
})

app.use((req, res) => {
  res.status(404).json({error:"Route not found"})
})

app.use((err, req, res, next) => {
  console.error(err)
  res.status(500).json({error:"Server error", message:err.message})
})

app.listen(port, () => {
  console.log(`Bonk server listening on port ${port}`)
})
