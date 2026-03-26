const db = require("../database/db");

// 유저 가져오기 (없으면 생성)
function getOrCreateUser(userId) {
  let user = db.prepare(
    "SELECT * FROM users WHERE userId = ?"
  ).get(userId);

  if (!user) {
    db.prepare(
      "INSERT INTO users (userId, gold, exp) VALUES (?, 0, 0)"
    ).run(userId);
    user = db.prepare(
      "SELECT * FROM users WHERE userId = ?"
    ).get(userId);
  }
  return user;
}

// 학교 데이터 기반 재화 갱신 (증분 방식 - 중복 지급 방지)
function applySchoolReward(userId, newAttendance, newAssignment) {
  const user = getOrCreateUser(userId);

  // 이전 스냅샷 가져오기
  const snapshot = db.prepare(
    "SELECT * FROM school_snapshots WHERE userId = ?"
  ).get(userId) || { attendanceCount: 0, assignmentCount: 0 };

  // 증가분만 계산
  const deltaAttendance = Math.max(0, newAttendance - snapshot.attendanceCount);
  const deltaAssignment = Math.max(0, newAssignment - snapshot.assignmentCount);

  const goldDelta = deltaAttendance * 100;
  const expDelta  = deltaAssignment * 50;

  // 재화 업데이트
  db.prepare(`
    UPDATE users
    SET gold = gold + ?, exp = exp + ?, updatedAt = datetime('now')
    WHERE userId = ?
  `).run(goldDelta, expDelta, userId);

  // 스냅샷 업데이트
  db.prepare(`
    INSERT INTO school_snapshots (userId, attendanceCount, assignmentCount, updatedAt)
    VALUES (?, ?, ?, datetime('now'))
    ON CONFLICT(userId) DO UPDATE SET
      attendanceCount = excluded.attendanceCount,
      assignmentCount = excluded.assignmentCount,
      updatedAt       = excluded.updatedAt
  `).run(userId, newAttendance, newAssignment);

  return db.prepare("SELECT * FROM users WHERE userId = ?").get(userId);
}

// 재화 차감 (아이템 구매 등)
function spendGold(userId, amount) {
  const user = getOrCreateUser(userId);

  if (user.gold < amount) {
    return { success: false, message: "골드 부족", current: user.gold };
  }

  db.prepare(`
    UPDATE users
    SET gold = gold - ?, updatedAt = datetime('now')
    WHERE userId = ?
  `).run(amount, userId);

  return {
    success: true,
    current: db.prepare("SELECT * FROM users WHERE userId = ?").get(userId)
  };
}

module.exports = { getOrCreateUser, applySchoolReward, spendGold };