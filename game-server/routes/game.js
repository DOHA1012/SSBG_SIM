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
    const result = applySchoolReward(userId, attendanceCount, assignmentCount);
    res.json({
      success: true,
      user: result.user,
      delta: result.delta,         // 변동분
      hasChange: result.hasChange  // 변동 있었는지 여부
    });
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

    // snapshot 가져오기
    let lastSnapshot = null;
    try {
      lastSnapshot = db.prepare(
        "SELECT * FROM login_snapshots WHERE userId = ?"
      ).get(userId);
    } catch (e) {
      console.log("snapshot table 없음 or 오류:", e.message);
    }

    const delta = {
      academicCurrency: lastSnapshot ? user.academicCurrency - lastSnapshot.academicCurrency : 0,
      extraCurrency:    lastSnapshot ? user.extraCurrency    - lastSnapshot.extraCurrency    : 0,
      idleCurrency:     lastSnapshot ? user.idleCurrency     - lastSnapshot.idleCurrency     : 0,
      exp:              lastSnapshot ? user.exp              - lastSnapshot.exp              : 0,
    };

    const hasChange = Object.values(delta).some(v => v > 0);

    //  snapshot 저장
    try {
      db.prepare(`
        INSERT INTO login_snapshots (userId, academicCurrency, extraCurrency, idleCurrency, exp)
        VALUES (?, ?, ?, ?, ?)
        ON CONFLICT(userId) DO UPDATE SET
          academicCurrency = excluded.academicCurrency,
          extraCurrency    = excluded.extraCurrency,
          idleCurrency     = excluded.idleCurrency,
          exp              = excluded.exp
      `).run(userId, user.academicCurrency, user.extraCurrency, user.idleCurrency, user.exp);
    } catch (e) {
      console.log("snapshot 저장 실패:", e.message);
    }

    res.json({
      success: true,
      user: user,      
      delta: delta,
      hasChange: hasChange
    });

  } catch (err) {
    console.error("LOGIN ERROR:", err);
    res.status(500).json({ error: err.message });
  }
});

module.exports = router;
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