const express = require("express");
const router = express.Router();
const {
  getAttendance,
  getAssignment
} = require("../services/schoolService");

// 보상 계산 함수
function calculateReward(attendance, assignment) {
  const attendanceCount = attendance.filter(
    a => a.status === "출석"
  ).length;

  const assignmentCount = assignment.filter(
    a => a.status === "제출"
  ).length;

  const gold = attendanceCount * 100;
  const exp = assignmentCount * 50;

  return { gold, exp, attendanceCount, assignmentCount };
}

// 핵심 API
router.post("/sync-eclass", async (req, res) => {
  const { userId } = req.body;

  try {
    const attendance = await getAttendance(userId);
    const assignment = await getAssignment(userId);

    const reward = calculateReward(attendance, assignment);

    res.json({
      userId,
      ...reward,
      message: "동기화 완료"
    });

  } catch (err) {
    res.status(500).json({
      error: "서버 오류",
      detail: err.message
    });
  }
});

module.exports = router;