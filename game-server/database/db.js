const Database = require("better-sqlite3");
const path = require("path");

const db = new Database(path.join(__dirname, "game.db"));

// 유저 테이블
db.exec(`
  CREATE TABLE IF NOT EXISTS users (
    userId      TEXT PRIMARY KEY,
    gold        INTEGER DEFAULT 0,
    exp         INTEGER DEFAULT 0,
    updatedAt   TEXT DEFAULT (datetime('now'))
  )
`);

// 학교 데이터 스냅샷 
db.exec(`
  CREATE TABLE IF NOT EXISTS school_snapshots (
    userId          TEXT PRIMARY KEY,
    attendanceCount INTEGER DEFAULT 0,
    assignmentCount INTEGER DEFAULT 0,
    updatedAt       TEXT DEFAULT (datetime('now'))
  )
`);

module.exports = db;