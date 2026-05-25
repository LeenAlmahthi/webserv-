const fs = require('fs');
const path = require('path');
const Ajv = require('ajv/dist/2020');
const addFormats = require('ajv-formats');

function loadJSON(relPath){
  const p = path.join(__dirname, relPath);
  const raw = fs.readFileSync(p, 'utf8');
  return JSON.parse(raw);
}

function validate(file){
  const schema = loadJSON('site-config.schema.json');
  const ajv = new Ajv({allErrors:true, strict:false});
  addFormats(ajv);
  const validate = ajv.compile(schema);
  const data = loadJSON(file);
  const ok = validate(data);
  if(ok){
    console.log(`[OK] ${file} conforms to schema`);
  } else {
    console.error(`[FAIL] ${file} has validation errors:`);
    console.error(validate.errors);
    process.exitCode = 1;
  }
}

['site-config.example.json','full-site-data.example.json'].forEach(validate);
