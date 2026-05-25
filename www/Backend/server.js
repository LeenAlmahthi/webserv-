const express = require('express');
const fs = require('fs').promises;
const path = require('path');
const bcrypt = require('bcrypt');
const Ajv = require('ajv/dist/2020');
const addFormats = require('ajv-formats');

const app = express();
const PORT = process.env.PORT || 3000;
const DATA_FILE = path.join(__dirname, 'users.json');
const SITE_CONFIGS_FILE = path.join(__dirname, 'site-configs.json');
const SITE_SCHEMA_FILE = path.join(__dirname, 'site-config.schema.json');
const FRONTEND_DIR = path.join(__dirname, '..', 'Frontend');

app.use(express.json());
app.use((req, res, next) => {
  res.header('Access-Control-Allow-Origin', '*');
  res.header('Access-Control-Allow-Headers', 'Content-Type');
  res.header('Access-Control-Allow-Methods', 'GET,POST,OPTIONS');
  if (req.method === 'OPTIONS') return res.sendStatus(204);
  next();
});
app.use(express.static(FRONTEND_DIR));

async function readUsers() {
  try {
    const raw = await fs.readFile(DATA_FILE, 'utf8');
    return JSON.parse(raw || '[]');
  } catch (err) {
    if (err.code === 'ENOENT') return [];
    throw err;
  }
}

async function writeUsers(users) {
  await fs.writeFile(DATA_FILE, JSON.stringify(users, null, 2), 'utf8');
}

app.post('/api/signup', async (req, res) => {
  try {
    console.log('POST /api/signup', req.body);
    const { name, email, password } = req.body || {};
    if (!email || !password) return res.status(400).json({ message: 'Email and password are required' });

    const normalized = String(email).trim().toLowerCase();

    const users = await readUsers();
    if (users.find(u => u.email === normalized)) return res.status(409).json({ message: 'User already exists' });

    const saltRounds = 10;
    const hash = await bcrypt.hash(password, saltRounds);

    const user = { id: Date.now(), name: name || '', email: normalized, passwordHash: hash, createdAt: new Date().toISOString() };
    users.push(user);
    await writeUsers(users);

    return res.status(201).json({ message: 'User created' });
  } catch (err) {
    console.error(err);
    return res.status(500).json({ message: 'Server error' });
  }
});

app.post('/api/login', async (req, res) => {
  try {
    console.log('POST /api/login', req.body);
    const { email, password } = req.body || {};
    if (!email || !password) return res.status(400).json({ message: 'Missing credentials' });

    const users = await readUsers();
    const user = users.find(u => u.email === String(email).trim().toLowerCase());
    if (!user) return res.status(401).json({ message: 'Invalid credentials' });

    const ok = await bcrypt.compare(password, user.passwordHash);
    if (!ok) return res.status(401).json({ message: 'Invalid credentials' });

    return res.json({ message: 'Login successful' });
  } catch (err) {
    console.error(err);
    return res.status(500).json({ message: 'Server error' });
  }
});

// Accept comprehensive site config payloads
app.post('/api/site-config', async (req, res) => {
  try {
    const rawSchema = await fs.readFile(SITE_SCHEMA_FILE, 'utf8');
    const schema = JSON.parse(rawSchema);
    const ajv = new Ajv({allErrors:true, strict:false});
    addFormats(ajv);
    const validate = ajv.compile(schema);
    const ok = validate(req.body);
    if(!ok){
      return res.status(400).json({ message: 'Invalid payload', errors: validate.errors });
    }
    let configs = [];
    try {
      const raw = await fs.readFile(SITE_CONFIGS_FILE, 'utf8');
      configs = JSON.parse(raw || '[]');
    } catch (err) {
      if (err.code !== 'ENOENT') throw err;
    }
    const stored = { id: Date.now(), ...req.body };
    configs.push(stored);
    await fs.writeFile(SITE_CONFIGS_FILE, JSON.stringify(configs, null, 2), 'utf8');
    return res.status(201).json({ message: 'Config stored', id: stored.id });
  } catch(err){
    console.error(err);
    return res.status(500).json({ message: 'Server error' });
  }
});

app.listen(PORT, () => {
  console.log(`Server running at http://localhost:${PORT}`);
});
