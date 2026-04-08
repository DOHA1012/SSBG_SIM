const express = require("express");
const router = express.Router();
const { getOrCreateUser, applySchoolReward, spendCurrency } = require("../services/userService");
const db = require("../database");

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
      delta: result.delta,
      hasChange: result.hasChange
    });
  } catch (err) {
    res.status(500).json({ error: err.message });
  }
});

// 2. 클라이언트 로그인
router.post("/login", (req, res) => {
  const { userId } = req.body;

  if (!userId) {
    return res.status(400).json({ error: "userId required" });
  }

  try {
    const user = getOrCreateUser(userId);

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

// 5. 하루 정산
router.post("/daily-summary", (req, res) => {
  const { userId } = req.body;

  if (!userId) {
    return res.status(400).json({ error: "userId required" });
  }

  try {
    const now = new Date();

    const nextReset = new Date(now);
    nextReset.setUTCHours(6, 0, 0, 0);
    if (now.getUTCHours() >= 6) {
      nextReset.setUTCDate(nextReset.getUTCDate() + 1);
    }

    const snapshot = db.prepare(
      "SELECT * FROM school_snapshots WHERE userId = ?"
    ).get(userId) || { attendanceCount: 0, assignmentCount: 0 };

    const schoolResult = applySchoolReward(
      userId,
      snapshot.attendanceCount,
      snapshot.assignmentCount
    );

    let playStats = {
      totalExp:             0,
      totalAcademicCurrency: 0,
      totalExtraCurrency:   0,
      totalIdleCurrency:    0,
      playTime:             0
    };
    try {
      playStats = db.prepare(`
        SELECT
          COALESCE(SUM(exp_gained), 0)               AS totalExp,
          COALESCE(SUM(academic_currency_gained), 0) AS totalAcademicCurrency,
          COALESCE(SUM(extra_currency_gained), 0)    AS totalExtraCurrency,
          COALESCE(SUM(idle_currency_gained), 0)     AS totalIdleCurrency,
          COALESCE(SUM(play_minutes), 0)             AS playTime
        FROM daily_play_log
        WHERE userId = ? AND date = date('now')
      `).get(userId);
    } catch (e) {
      console.log("daily_play_log 오류:", e.message);
    }

    res.json({
      success: true,
      time: {
        summaryAt:         now.toISOString(),
        summaryDateLabel:  now.toISOString().slice(0, 10),
        nextResetAt:       nextReset.toISOString(),
        secondsUntilReset: Math.floor((nextReset - now) / 1000),
      },
      attendanceDelta:             schoolResult.delta?.extraCurrency ?? 0,
      assignmentDelta:             schoolResult.delta?.exp           ?? 0,
      bHasSchoolChange:            schoolResult.hasChange            ?? false,
      totalExpGained:              playStats.totalExp,
      totalAcademicCurrencyGained: playStats.totalAcademicCurrency,
      totalExtraCurrencyGained:    playStats.totalExtraCurrency,
      totalIdleCurrencyGained:     playStats.totalIdleCurrency,
      playTimeMinutes:             playStats.playTime,
    });

  } catch (err) {
    console.error("DAILY SUMMARY ERROR:", err);
    res.status(500).json({ error: err.message });
  }
});

module.exports = router;