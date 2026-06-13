const Database = require('better-sqlite3');
const path = require('path');

const dbPath = path.join(__dirname, 'data.db');
const db = new Database(dbPath);

//创建 messages 表
db.exec(`
  CREATE TABLE IF NOT EXISTS messages (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    content TEXT NOT NULL,
    created_at DATETIME DEFAULT (datetime('now', 'localtime'))
  )
`);

//访客计数表
db.exec(`
  CREATE TABLE IF NOT EXISTS visitor_count (
    id INTEGER PRIMARY KEY CHECK (id = 1),
    count INTEGER DEFAULT 0
  )
`);

// 确保有一行初始数据
db.exec(`INSERT OR IGNORE INTO visitor_count (id, count) VALUES (1, 0)`);

module.exports = db;
