const express = require("express");
const router = express.Router();
const { getOrCreateUser, applySchoolReward, spendGold } = require("../services/userService");

// 1. 학교서버 → 게임서버 웹훅 (학교서버가 푸시)
router.post("/school-webhook", async (req, res) => {
  const { userId, attendanceCount, assignmentCount } = req.body;

  if (!userId) {
    return res.status(400).json({ error: "userId 필요" });
  }

  try {
    const updated = applySchoolReward(userId, attendanceCount, assignmentCount);
    console.log(`[Webhook] ${userId} 재화 갱신 완료:`, updated);
    res.json({ success: true, user: updated });
  } catch (err) {
    res.status(500).json({ error: err.message });
  }
});

// 2. 클라이언트 접속 시 유저 데이터 요청
router.post("/login", (req, res) => {
  const { userId } = req.body;

  if (!userId) {
    return res.status(400).json({ error: "userId 필요" });
  }

  try {
    const user = getOrCreateUser(userId);
    res.json({ success: true, user });
  } catch (err) {
    res.status(500).json({ error: err.message });
  }
});

// 3. 클라이언트 → 재화 사용 요청
router.post("/spend-gold", (req, res) => {
  const { userId, amount } = req.body;

  if (!userId || amount == null) {
    return res.status(400).json({ error: "userId, amount 필요" });
  }

  try {
    const result = spendGold(userId, amount);
    res.json(result);
  } catch (err) {
    res.status(500).json({ error: err.message });
  }
});

// 4. 유저 현재 상태 조회 
router.get("/user/:userId", (req, res) => {
  const { userId } = req.params;

  try {
    const user = getOrCreateUser(userId);
    res.json({ success: true, user });
  } catch (err) {
    res.status(500).json({ error: err.message });
  }
});


module.exports = router;