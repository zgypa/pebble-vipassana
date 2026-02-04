// Schedule engine implementation with student and server timetables.
#include "schedule.h"

static const Activity k_student_day_minus_one[] = {
  {5 * 60 + 55, "Chanting", ACTIVITY_OTHER},
  {6 * 60 + 30, "Breakfast", ACTIVITY_MEAL},
  {7 * 60 + 0, "Meeting", ACTIVITY_INFO},
  {7 * 60 + 30, "Group sitting", ACTIVITY_MEDITATION},
  {8 * 60 + 30, "Work Period", ACTIVITY_WORK},
  {12 * 60 + 0, "Lunch", ACTIVITY_MEAL},
  {13 * 60 + 0, "Rest", ACTIVITY_REST},
  {14 * 60 + 30, "Group sitting", ACTIVITY_MEDITATION},
  {15 * 60 + 30, "Work Period", ACTIVITY_WORK},
  {18 * 60 + 0, "Dinner", ACTIVITY_MEAL},
  {19 * 60 + 30, "Group Sitting", ACTIVITY_MEDITATION},
  {20 * 60 + 30, "Metta", ACTIVITY_MEDITATION},
  {22 * 60 + 0, "Lights out", ACTIVITY_SLEEP},
};

static const Activity k_student_day_zero[] = {
  {5 * 60 + 55, "Chanting", ACTIVITY_OTHER},
  {6 * 60 + 30, "Breakfast", ACTIVITY_MEAL},
  {7 * 60 + 0, "Meeting", ACTIVITY_INFO},
  {7 * 60 + 30, "Group sitting", ACTIVITY_MEDITATION},
  {8 * 60 + 30, "Prepare Kitchen", ACTIVITY_WORK},
  {10 * 60 + 30, "Prepare Registration", ACTIVITY_WORK},
  {11 * 60 + 0, "Lunch", ACTIVITY_MEAL},
  {13 * 60 + 0, "Group Sitting", ACTIVITY_MEDITATION},
  {14 * 60 + 0, "Registration", ACTIVITY_WORK},
  {14 * 60 + 30, "Food Preparations", ACTIVITY_WORK},
  {15 * 60 + 30, "Kitchen Meeting", ACTIVITY_INFO},
  {18 * 60 + 0, "Dinner", ACTIVITY_MEAL},
  {19 * 60 + 0, "Information", ACTIVITY_INFO},
  {20 * 60 + 0, "Course Starts", ACTIVITY_OTHER},
  {22 * 60 + 0, "Lights out", ACTIVITY_SLEEP},
};

static const Activity k_student_day_one[] = {
  {4 * 60 + 0, "Wake up", ACTIVITY_OTHER},
  {4 * 60 + 30, "Meditation", ACTIVITY_MEDITATION},
  {6 * 60 + 30, "Breakfast", ACTIVITY_MEAL},
  {8 * 60 + 0, "Group sitting", ACTIVITY_MEDITATION},
  {9 * 60 + 0, "Meditation", ACTIVITY_MEDITATION},
  {11 * 60 + 0, "Lunch", ACTIVITY_MEAL},
  {12 * 60 + 0, "Interviews", ACTIVITY_INFO},
  {13 * 60 + 0, "Meditation", ACTIVITY_MEDITATION},
  {14 * 60 + 30, "Group sitting", ACTIVITY_MEDITATION},
  {15 * 60 + 30, "Meditation", ACTIVITY_MEDITATION},
  {17 * 60 + 0, "Tea", ACTIVITY_MEAL},
  {18 * 60 + 0, "Group sitting", ACTIVITY_MEDITATION},
  {19 * 60 + 0, "Discourse", ACTIVITY_DISCOURSE},
  {20 * 60 + 15, "Group sitting", ACTIVITY_MEDITATION},
  {21 * 60 + 0, "Questions", ACTIVITY_INFO},
  {22 * 60 + 0, "Lights out", ACTIVITY_SLEEP},
};

static const Activity k_student_day_four[] = {
  {4 * 60 + 0, "Wake up", ACTIVITY_OTHER},
  {4 * 60 + 30, "Meditation", ACTIVITY_MEDITATION},
  {6 * 60 + 30, "Breakfast", ACTIVITY_MEAL},
  {8 * 60 + 0, "Group sitting", ACTIVITY_MEDITATION},
  {9 * 60 + 0, "Meditation", ACTIVITY_MEDITATION},
  {11 * 60 + 0, "Lunch", ACTIVITY_MEAL},
  {12 * 60 + 0, "Interviews", ACTIVITY_INFO},
  {13 * 60 + 0, "Meditation", ACTIVITY_MEDITATION},
  {14 * 60 + 0, "Group sitting", ACTIVITY_MEDITATION},
  {15 * 60 + 0, "Vipassana Teaching", ACTIVITY_INFO},
  {17 * 60 + 0, "Tea", ACTIVITY_MEAL},
  {18 * 60 + 0, "Group sitting", ACTIVITY_MEDITATION},
  {19 * 60 + 0, "Discourse", ACTIVITY_DISCOURSE},
  {20 * 60 + 15, "Group sitting", ACTIVITY_MEDITATION},
  {21 * 60 + 0, "Questions", ACTIVITY_INFO},
  {22 * 60 + 0, "Lights out", ACTIVITY_SLEEP},
};

static const Activity k_student_day_ten[] = {
  {4 * 60 + 0, "Wake up", ACTIVITY_OTHER},
  {4 * 60 + 30, "Meditation", ACTIVITY_MEDITATION},
  {6 * 60 + 30, "Breakfast", ACTIVITY_MEAL},
  {8 * 60 + 0, "Group sitting", ACTIVITY_MEDITATION},
  {9 * 60 + 0, "Meditation", ACTIVITY_MEDITATION},
  {10 * 60 + 10, "Noble Silence ends", ACTIVITY_INFO},
  {11 * 60 + 0, "Lunch", ACTIVITY_MEAL},
  {12 * 60 + 0, "Interviews", ACTIVITY_INFO},
  {13 * 60 + 0, "Rest", ACTIVITY_REST},
  {14 * 60 + 30, "Group sitting", ACTIVITY_MEDITATION},
  {15 * 60 + 50, "Rest", ACTIVITY_REST},
  {16 * 60 + 0, "Information", ACTIVITY_INFO},
  {17 * 60 + 0, "Dinner", ACTIVITY_MEAL},
  {18 * 60 + 0, "Group sitting", ACTIVITY_MEDITATION},
  {19 * 60 + 0, "Discourse", ACTIVITY_DISCOURSE},
  {20 * 60 + 15, "Rest", ACTIVITY_REST},
  {22 * 60 + 0, "Lights out", ACTIVITY_SLEEP},
};

static const Activity k_student_day_eleven[] = {
  {4 * 60 + 0, "Wake up", ACTIVITY_OTHER},
  {4 * 60 + 30, "Group Sitting", ACTIVITY_MEDITATION},
  {6 * 60 + 30, "Cleaning", ACTIVITY_WORK},
  {7 * 60 + 0, "Breakfast", ACTIVITY_MEAL},
  {7 * 60 + 30, "Cleaning", ACTIVITY_WORK},
  {8 * 60 + 50, "Bus Leaves", ACTIVITY_OTHER},
  {9 * 60 + 15, "Meeting", ACTIVITY_INFO},
  {10 * 60 + 0, "Group Sitting", ACTIVITY_MEDITATION},
  {11 * 60 + 0, "Work Period", ACTIVITY_WORK},
  {12 * 60 + 0, "Lunch", ACTIVITY_MEAL},
  {13 * 60 + 0, "Rest", ACTIVITY_REST},
  {14 * 60 + 30, "Group sitting", ACTIVITY_MEDITATION},
  {15 * 60 + 30, "Work Period", ACTIVITY_WORK},
  {18 * 60 + 0, "Dinner", ACTIVITY_MEAL},
  {19 * 60 + 30, "Group sitting", ACTIVITY_MEDITATION},
  {20 * 60 + 30, "Metta", ACTIVITY_MEDITATION},
  {22 * 60 + 0, "Lights out", ACTIVITY_SLEEP},
};

static const Activity k_server_day_minus_one[] = {
  {5 * 60 + 30, "Wake up", ACTIVITY_OTHER},
  {6 * 60 + 0, "Morning sit", ACTIVITY_MEDITATION},
  {7 * 60 + 0, "Breakfast", ACTIVITY_MEAL},
  {7 * 60 + 45, "Service Prep", ACTIVITY_WORK},
  {9 * 60 + 0, "Work Period", ACTIVITY_WORK},
  {12 * 60 + 0, "Lunch", ACTIVITY_MEAL},
  {13 * 60 + 0, "Rest", ACTIVITY_REST},
  {14 * 60 + 0, "Support", ACTIVITY_WORK},
  {17 * 60 + 0, "Dinner", ACTIVITY_MEAL},
  {18 * 60 + 0, "Evening sit", ACTIVITY_MEDITATION},
  {20 * 60 + 0, "Meeting", ACTIVITY_INFO},
  {22 * 60 + 0, "Lights out", ACTIVITY_SLEEP},
};

static const Activity k_server_day_zero[] = {
  {5 * 60 + 30, "Wake up", ACTIVITY_OTHER},
  {6 * 60 + 0, "Meditation", ACTIVITY_MEDITATION},
  {7 * 60 + 0, "Breakfast", ACTIVITY_MEAL},
  {8 * 60 + 0, "Service Work", ACTIVITY_WORK},
  {11 * 60 + 0, "Lunch", ACTIVITY_MEAL},
  {12 * 60 + 0, "Rest", ACTIVITY_REST},
  {13 * 60 + 30, "Registration", ACTIVITY_WORK},
  {16 * 60 + 0, "Prep", ACTIVITY_WORK},
  {18 * 60 + 0, "Dinner", ACTIVITY_MEAL},
  {19 * 60 + 30, "Orientation", ACTIVITY_INFO},
  {22 * 60 + 0, "Lights out", ACTIVITY_SLEEP},
};

static const Activity k_server_day_one[] = {
  {4 * 60 + 30, "Wake up", ACTIVITY_OTHER},
  {5 * 60 + 0, "Meditation", ACTIVITY_MEDITATION},
  {6 * 60 + 30, "Breakfast", ACTIVITY_MEAL},
  {7 * 60 + 30, "Kitchen", ACTIVITY_WORK},
  {9 * 60 + 30, "Service Work", ACTIVITY_WORK},
  {11 * 60 + 0, "Lunch", ACTIVITY_MEAL},
  {12 * 60 + 0, "Rest", ACTIVITY_REST},
  {13 * 60 + 0, "Service Work", ACTIVITY_WORK},
  {17 * 60 + 0, "Tea", ACTIVITY_MEAL},
  {18 * 60 + 0, "Meditation", ACTIVITY_MEDITATION},
  {19 * 60 + 0, "Discourse", ACTIVITY_DISCOURSE},
  {20 * 60 + 15, "Service Work", ACTIVITY_WORK},
  {22 * 60 + 0, "Lights out", ACTIVITY_SLEEP},
};

static const Activity k_server_day_four[] = {
  {4 * 60 + 30, "Wake up", ACTIVITY_OTHER},
  {5 * 60 + 0, "Meditation", ACTIVITY_MEDITATION},
  {6 * 60 + 30, "Breakfast", ACTIVITY_MEAL},
  {7 * 60 + 30, "Service Work", ACTIVITY_WORK},
  {10 * 60 + 0, "Meeting", ACTIVITY_INFO},
  {11 * 60 + 0, "Lunch", ACTIVITY_MEAL},
  {12 * 60 + 0, "Rest", ACTIVITY_REST},
  {13 * 60 + 0, "Support", ACTIVITY_WORK},
  {17 * 60 + 0, "Tea", ACTIVITY_MEAL},
  {18 * 60 + 0, "Meditation", ACTIVITY_MEDITATION},
  {19 * 60 + 0, "Discourse", ACTIVITY_DISCOURSE},
  {20 * 60 + 15, "Service Work", ACTIVITY_WORK},
  {22 * 60 + 0, "Lights out", ACTIVITY_SLEEP},
};

static const Activity k_server_day_ten[] = {
  {4 * 60 + 30, "Wake up", ACTIVITY_OTHER},
  {5 * 60 + 0, "Meditation", ACTIVITY_MEDITATION},
  {6 * 60 + 30, "Breakfast", ACTIVITY_MEAL},
  {7 * 60 + 30, "Service Work", ACTIVITY_WORK},
  {10 * 60 + 0, "Meeting", ACTIVITY_INFO},
  {11 * 60 + 0, "Lunch", ACTIVITY_MEAL},
  {12 * 60 + 0, "Rest", ACTIVITY_REST},
  {13 * 60 + 30, "Support", ACTIVITY_WORK},
  {17 * 60 + 0, "Dinner", ACTIVITY_MEAL},
  {18 * 60 + 0, "Meditation", ACTIVITY_MEDITATION},
  {19 * 60 + 0, "Discourse", ACTIVITY_DISCOURSE},
  {20 * 60 + 15, "Rest", ACTIVITY_REST},
  {22 * 60 + 0, "Lights out", ACTIVITY_SLEEP},
};

static const Activity k_server_day_eleven[] = {
  {5 * 60 + 0, "Wake up", ACTIVITY_OTHER},
  {5 * 60 + 30, "Meditation", ACTIVITY_MEDITATION},
  {6 * 60 + 30, "Cleaning", ACTIVITY_WORK},
  {7 * 60 + 30, "Breakfast", ACTIVITY_MEAL},
  {8 * 60 + 30, "Pack", ACTIVITY_OTHER},
  {10 * 60 + 0, "Meeting", ACTIVITY_INFO},
  {11 * 60 + 0, "Lunch", ACTIVITY_MEAL},
  {12 * 60 + 30, "Rest", ACTIVITY_REST},
  {14 * 60 + 0, "Service Work", ACTIVITY_WORK},
  {17 * 60 + 0, "Dinner", ACTIVITY_MEAL},
  {18 * 60 + 0, "Metta", ACTIVITY_MEDITATION},
  {20 * 60 + 0, "Lights out", ACTIVITY_SLEEP},
};

typedef struct {
  int day;
  const Activity *activities;
  size_t count;
} DayTable;

static DaySchedule schedule_from_table(const DayTable *table, size_t count, int day) {
  for (size_t i = 0; i < count; i++) {
    if (table[i].day == day) {
      return (DaySchedule){
        .activities = table[i].activities,
        .count = table[i].count,
      };
    }
  }
  return (DaySchedule){
    .activities = k_student_day_one,
    .count = ARRAY_LENGTH(k_student_day_one),
  };
}

DaySchedule schedule_get_day(CourseType course_type, CourseRole role, int day) {
  if (course_type != COURSE_TYPE_TEN_DAY) {
    return (DaySchedule){
      .activities = k_student_day_one,
      .count = ARRAY_LENGTH(k_student_day_one),
    };
  }

  if (day == 12) {
    day = -1;
  }

  if (role == COURSE_ROLE_SERVER) {
    static const DayTable k_server_table[] = {
      {-1, k_server_day_minus_one, ARRAY_LENGTH(k_server_day_minus_one)},
      {0, k_server_day_zero, ARRAY_LENGTH(k_server_day_zero)},
      {1, k_server_day_one, ARRAY_LENGTH(k_server_day_one)},
      {4, k_server_day_four, ARRAY_LENGTH(k_server_day_four)},
      {10, k_server_day_ten, ARRAY_LENGTH(k_server_day_ten)},
      {11, k_server_day_eleven, ARRAY_LENGTH(k_server_day_eleven)},
    };
    return schedule_from_table(k_server_table, ARRAY_LENGTH(k_server_table), day);
  }

  static const DayTable k_student_table[] = {
    {-1, k_student_day_minus_one, ARRAY_LENGTH(k_student_day_minus_one)},
    {0, k_student_day_zero, ARRAY_LENGTH(k_student_day_zero)},
    {1, k_student_day_one, ARRAY_LENGTH(k_student_day_one)},
    {4, k_student_day_four, ARRAY_LENGTH(k_student_day_four)},
    {10, k_student_day_ten, ARRAY_LENGTH(k_student_day_ten)},
    {11, k_student_day_eleven, ARRAY_LENGTH(k_student_day_eleven)},
  };

  return schedule_from_table(k_student_table, ARRAY_LENGTH(k_student_table), day);
}

int schedule_current_index(const DaySchedule *schedule, int minutes) {
  if (schedule->count == 0) {
    return 0;
  }

  if (minutes < schedule->activities[0].minutes ||
      minutes >= schedule->activities[schedule->count - 1].minutes) {
    return (int)schedule->count - 1;
  }

  for (size_t i = 0; i < schedule->count; i++) {
    if (minutes < schedule->activities[i].minutes) {
      return (int)i - 1;
    }
  }

  return (int)schedule->count - 1;
}

int schedule_next_index(const DaySchedule *schedule, int current_index) {
  if (schedule->count == 0) {
    return 0;
  }

  if (current_index >= (int)schedule->count - 1) {
    return 0;
  }

  return current_index + 1;
}

int schedule_minutes_until_next_kind(const DaySchedule *schedule,
                                     int current_index,
                                     ActivityKind kind,
                                     int minutes_now,
                                     int *out_day_offset) {
  if (out_day_offset) {
    *out_day_offset = 0;
  }

  if (schedule->count == 0) {
    return -1;
  }

  for (int i = current_index; i < (int)schedule->count; i++) {
    if (schedule->activities[i].kind == kind && schedule->activities[i].minutes >= minutes_now) {
      return schedule->activities[i].minutes - minutes_now;
    }
  }

  if (out_day_offset) {
    *out_day_offset = 1;
  }

  return -1;
}

const char *schedule_kind_label(ActivityKind kind) {
  switch (kind) {
    case ACTIVITY_MEDITATION:
      return "Meditation";
    case ACTIVITY_REST:
      return "Rest";
    case ACTIVITY_WORK:
      return "Work";
    case ACTIVITY_MEAL:
      return "Meal";
    case ACTIVITY_DISCOURSE:
      return "Discourse";
    case ACTIVITY_INFO:
      return "Info";
    case ACTIVITY_SLEEP:
      return "Sleep";
    case ACTIVITY_OTHER:
    default:
      return "Other";
  }
}
