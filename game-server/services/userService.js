const db = require("../database/db");

// 재화 지급 설정 
const REWARD_CONFIG = {
  attendance: {
    extraCurrency: 100,
    exp: 30,
  },
  assignment: {
    exp: 50,
  }
};

// 유저 가져오기 
function getOrCreateUser(userId) {
  let user = db.prepare(
    "SELECT * FROM users WHERE userId = ?"
  ).get(userId);

  if (!user) {
    db.prepare(
      `INSERT INTO users (userId, academicCurrency, extraCurrency, idleCurrency, exp)
       VALUES (?, 0, 0, 0, 0)`
    ).run(userId);
    user = db.prepare(
      "SELECT * FROM users WHERE userId = ?"
    ).get(userId);
  }
  return user;
}

// 학교 데이터 기반 재화 갱신 
function applySchoolReward(userId, newAttendance, newAssignment) {
  getOrCreateUser(userId);

  const snapshot = db.prepare(
    "SELECT * FROM school_snapshots WHERE userId = ?"
  ).get(userId) || { attendanceCount: 0, assignmentCount: 0 };

  const deltaAttendance = Math.max(0, newAttendance - snapshot.attendanceCount);
  const deltaAssignment = Math.max(0, newAssignment - snapshot.assignmentCount);

  const delta = {
    academicCurrency: 0,
    extraCurrency:    deltaAttendance * (REWARD_CONFIG.attendance.extraCurrency || 0),
    idleCurrency:     0,
    exp:              deltaAttendance * (REWARD_CONFIG.attendance.exp || 0)
                    + deltaAssignment * (REWARD_CONFIG.assignment.exp || 0),
  };

  db.prepare(`
    UPDATE users
    SET academicCurrency = academicCurrency + ?,
        extraCurrency    = extraCurrency    + ?,
        idleCurrency     = idleCurrency     + ?,
        exp              = exp              + ?,
        updatedAt        = datetime('now')
    WHERE userId = ?
  `).run(delta.academicCurrency, delta.extraCurrency, delta.idleCurrency, delta.exp, userId);

  db.prepare(`
    INSERT INTO school_snapshots (userId, attendanceCount, assignmentCount, updatedAt)
    VALUES (?, ?, ?, datetime('now'))
    ON CONFLICT(userId) DO UPDATE SET
      attendanceCount = excluded.attendanceCount,
      assignmentCount = excluded.assignmentCount,
      updatedAt       = excluded.updatedAt
  `).run(userId, newAttendance, newAssignment);

  const updated = db.prepare("SELECT * FROM users WHERE userId = ?").get(userId);

  // 변동분 함께 반환
  const hasChange = Object.values(delta).some(v => v > 0);
  return { user: updated, delta, hasChange };
}

// 재화 차감 (범용)
function spendCurrency(userId, currencyType, amount) {
  const validTypes = ["academicCurrency", "extraCurrency", "idleCurrency", "exp"];

  if (!validTypes.includes(currencyType)) {
    return { success: false, message: "Invalid currency type" };
  }

  const user = getOrCreateUser(userId);

  if (user[currencyType] < amount) {
    return { success: false, message: "Not enough currency", current: user };
  }

  db.prepare(`
    UPDATE users
    SET ${currencyType} = ${currencyType} - ?,
        updatedAt = datetime('now')
    WHERE userId = ?
  `).run(amount, userId);

  return {
    success: true,
    current: db.prepare("SELECT * FROM users WHERE userId = ?").get(userId)
  };
}

module.exports = { getOrCreateUser, applySchoolReward, spendCurrency };