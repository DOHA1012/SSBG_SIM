const express = require("express");
const router = express.Router();
const data = require("../data/mockData.json");

// 출석
router.get("/attendance", (req, res) => {
  const { userId } = req.query;

  res.json({
    userId,
    attendance: data.attendance
  });
});

// 과제
router.get("/assignment", (req, res) => {
  const { userId } = req.query;

  res.json({
    userId,
    assignment: data.assignment
  });
});

module.exports = router;