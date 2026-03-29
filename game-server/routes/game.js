const express = require("express");
const router = express.Router();
const { getOrCreateUser, applySchoolReward, spendCurrency } = require("../services/userService");

// 1. 학교서버 → 게임서버 웹훅
router.post("/school-webhook", (req, res) => {
  const { userId, attendanceCount, assignmentCount } = req.body;

  if (!userId) {
    return res.status(400).json({ error: "userId required" });
  }

  try {
    const updated = applySchoolReward(userId, attendanceCount, assignmentCount);
    console.log(`[Webhook] ${userId} updated:`, updated);
    res.json({ success: true, user: updated });
  } catch (err) {
    res.status(500).json({ error: err.message });
  }
});

// 2. 클라이언트 로그인 (접속 시 유저 데이터 수신)
router.post("/login", (req, res) => {
  const { userId } = req.body;

  if (!userId) {
    return res.status(400).json({ error: "userId required" });
  }

  try {
    const user = getOrCreateUser(userId);
    res.json({ success: true, user });
  } catch (err) {
    res.status(500).json({ error: err.message });
  }
});

// 3. 재화 차감 요청
router.post("/spend-currency", (req, res) => {
  const { userId, currencyType, amount } = req.body;

  if (!userId || !currencyType || amount == null) {
    return res.status(400).json({ error: "userId, currencyType, amount required" });
  }

  try {
    const result = spendCurrency(userId, currencyType, amount);
    res.json(result);
  } catch (err) {
    res.status(500).json({ error: err.message });
  }
});


// 4. 유저 상태 조회
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