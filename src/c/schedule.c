// Schedule engine implementation with student and server timetables.
#include "schedule.h"

#define ACT(time, text, kind) {time, text, kind, MEDITATION_NONE}
#define ACT_MED(time, text) {time, text, ACTIVITY_MEDITATION, MEDITATION_NONE}
#define ACT_METTA(time, text) {time, text, ACTIVITY_MEDITATION, MEDITATION_METTA}

static const Activity k_student_day_minus_one[] = {
  ACT(5 * 60 + 55, "Chanting", ACTIVITY_OTHER),
  ACT(6 * 60 + 30, "Breakfast", ACTIVITY_MEAL),
  ACT(7 * 60 + 0, "Meeting", ACTIVITY_INFO),
  ACT_MED(7 * 60 + 30, "Group sitting"),
  ACT(8 * 60 + 30, "Work Period", ACTIVITY_WORK),
  ACT(12 * 60 + 0, "Lunch", ACTIVITY_MEAL),
  ACT(13 * 60 + 0, "Rest", ACTIVITY_REST),
  ACT_MED(14 * 60 + 30, "Group sitting"),
  ACT(15 * 60 + 30, "Work Period", ACTIVITY_WORK),
  ACT(18 * 60 + 0, "Dinner", ACTIVITY_MEAL),
  ACT_MED(19 * 60 + 30, "Group Sitting"),
  ACT_METTA(20 * 60 + 30, "Metta"),
  ACT(22 * 60 + 0, "Lights out", ACTIVITY_SLEEP),
};

static const Activity k_student_day_zero[] = {
  ACT(5 * 60 + 55, "Chanting", ACTIVITY_OTHER),
  ACT(6 * 60 + 30, "Breakfast", ACTIVITY_MEAL),
  ACT(7 * 60 + 0, "Meeting", ACTIVITY_INFO),
  ACT_MED(7 * 60 + 30, "Group sitting"),
  ACT(8 * 60 + 30, "Prepare Kitchen", ACTIVITY_WORK),
  ACT(10 * 60 + 30, "Prepare Registration", ACTIVITY_WORK),
  ACT(11 * 60 + 0, "Lunch", ACTIVITY_MEAL),
  ACT_MED(13 * 60 + 0, "Group Sitting"),
  ACT(14 * 60 + 0, "Registration", ACTIVITY_WORK),
  ACT(14 * 60 + 30, "Food Preparations", ACTIVITY_WORK),
  ACT(15 * 60 + 30, "Kitchen Meeting", ACTIVITY_INFO),
  ACT(18 * 60 + 0, "Dinner", ACTIVITY_MEAL),
  ACT(19 * 60 + 0, "Information", ACTIVITY_INFO),
  ACT(20 * 60 + 0, "Course Starts", ACTIVITY_OTHER),
  ACT(22 * 60 + 0, "Lights out", ACTIVITY_SLEEP),
};

static const Activity k_student_day_one[] = {
  ACT(4 * 60 + 0, "Wake up", ACTIVITY_OTHER),
  ACT_MED(4 * 60 + 30, "Meditation"),
  ACT(6 * 60 + 30, "Breakfast", ACTIVITY_MEAL),
  ACT_MED(8 * 60 + 0, "Group sitting"),
  ACT_MED(9 * 60 + 0, "Meditation"),
  ACT(11 * 60 + 0, "Lunch", ACTIVITY_MEAL),
  ACT(12 * 60 + 0, "Interviews", ACTIVITY_INFO),
  ACT_MED(13 * 60 + 0, "Meditation"),
  ACT_MED(14 * 60 + 30, "Group sitting"),
  ACT_MED(15 * 60 + 30, "Meditation"),
  ACT(17 * 60 + 0, "Tea", ACTIVITY_MEAL),
  ACT_MED(18 * 60 + 0, "Group sitting"),
  ACT(19 * 60 + 0, "Discourse", ACTIVITY_DISCOURSE),
  ACT_MED(20 * 60 + 15, "Group sitting"),
  ACT(21 * 60 + 0, "Questions", ACTIVITY_INFO),
  ACT(22 * 60 + 0, "Lights out", ACTIVITY_SLEEP),
};

static const Activity k_student_day_four[] = {
  ACT(4 * 60 + 0, "Wake up", ACTIVITY_OTHER),
  ACT_MED(4 * 60 + 30, "Meditation"),
  ACT(6 * 60 + 30, "Breakfast", ACTIVITY_MEAL),
  ACT_MED(8 * 60 + 0, "Group sitting"),
  ACT_MED(9 * 60 + 0, "Meditation"),
  ACT(11 * 60 + 0, "Lunch", ACTIVITY_MEAL),
  ACT(12 * 60 + 0, "Interviews", ACTIVITY_INFO),
  ACT_MED(13 * 60 + 0, "Meditation"),
  ACT_MED(14 * 60 + 0, "Group sitting"),
  ACT(15 * 60 + 0, "Vipassana Teaching", ACTIVITY_INFO),
  ACT(17 * 60 + 0, "Tea", ACTIVITY_MEAL),
  ACT_MED(18 * 60 + 0, "Group sitting"),
  ACT(19 * 60 + 0, "Discourse", ACTIVITY_DISCOURSE),
  ACT_MED(20 * 60 + 15, "Group sitting"),
  ACT(21 * 60 + 0, "Questions", ACTIVITY_INFO),
  ACT(22 * 60 + 0, "Lights out", ACTIVITY_SLEEP),
};

static const Activity k_student_day_ten[] = {
  ACT(4 * 60 + 0, "Wake up", ACTIVITY_OTHER),
  ACT_MED(4 * 60 + 30, "Meditation"),
  ACT(6 * 60 + 30, "Breakfast", ACTIVITY_MEAL),
  ACT_MED(8 * 60 + 0, "Group sitting"),
  ACT_MED(9 * 60 + 0, "Meditation"),
  ACT(10 * 60 + 10, "Noble Silence ends", ACTIVITY_INFO),
  ACT(11 * 60 + 0, "Lunch", ACTIVITY_MEAL),
  ACT(12 * 60 + 0, "Interviews", ACTIVITY_INFO),
  ACT(13 * 60 + 0, "Rest", ACTIVITY_REST),
  ACT_MED(14 * 60 + 30, "Group sitting"),
  ACT(15 * 60 + 50, "Rest", ACTIVITY_REST),
  ACT(16 * 60 + 0, "Information", ACTIVITY_INFO),
  ACT(17 * 60 + 0, "Dinner", ACTIVITY_MEAL),
  ACT_MED(18 * 60 + 0, "Group sitting"),
  ACT(19 * 60 + 0, "Discourse", ACTIVITY_DISCOURSE),
  ACT(20 * 60 + 15, "Rest", ACTIVITY_REST),
  ACT(22 * 60 + 0, "Lights out", ACTIVITY_SLEEP),
};

static const Activity k_student_day_eleven[] = {
  ACT(4 * 60 + 0, "Wake up", ACTIVITY_OTHER),
  ACT_MED(4 * 60 + 30, "Group Sitting"),
  ACT(6 * 60 + 30, "Cleaning", ACTIVITY_WORK),
  ACT(7 * 60 + 0, "Breakfast", ACTIVITY_MEAL),
  ACT(7 * 60 + 30, "Cleaning", ACTIVITY_WORK),
  ACT(8 * 60 + 50, "Bus Leaves", ACTIVITY_OTHER),
  ACT(9 * 60 + 15, "Meeting", ACTIVITY_INFO),
  ACT_MED(10 * 60 + 0, "Group Sitting"),
  ACT(11 * 60 + 0, "Work Period", ACTIVITY_WORK),
  ACT(12 * 60 + 0, "Lunch", ACTIVITY_MEAL),
  ACT(13 * 60 + 0, "Rest", ACTIVITY_REST),
  ACT_MED(14 * 60 + 30, "Group sitting"),
  ACT(15 * 60 + 30, "Work Period", ACTIVITY_WORK),
  ACT(18 * 60 + 0, "Dinner", ACTIVITY_MEAL),
  ACT_MED(19 * 60 + 30, "Group sitting"),
  ACT_METTA(20 * 60 + 30, "Metta"),
  ACT(22 * 60 + 0, "Lights out", ACTIVITY_SLEEP),
};

static const Activity k_server_day_minus_one[] = {
  ACT(5 * 60 + 30, "Wake up", ACTIVITY_OTHER),
  ACT_MED(6 * 60 + 0, "Morning sit"),
  ACT(7 * 60 + 0, "Breakfast", ACTIVITY_MEAL),
  ACT(7 * 60 + 45, "Service Prep", ACTIVITY_WORK),
  ACT(9 * 60 + 0, "Work Period", ACTIVITY_WORK),
  ACT(12 * 60 + 0, "Lunch", ACTIVITY_MEAL),
  ACT(13 * 60 + 0, "Rest", ACTIVITY_REST),
  ACT(14 * 60 + 0, "Support", ACTIVITY_WORK),
  ACT(17 * 60 + 0, "Dinner", ACTIVITY_MEAL),
  ACT_MED(18 * 60 + 0, "Evening sit"),
  ACT(20 * 60 + 0, "Meeting", ACTIVITY_INFO),
  ACT(22 * 60 + 0, "Lights out", ACTIVITY_SLEEP),
};

static const Activity k_server_day_zero[] = {
  ACT(5 * 60 + 30, "Wake up", ACTIVITY_OTHER),
  ACT_MED(6 * 60 + 0, "Meditation"),
  ACT(7 * 60 + 0, "Breakfast", ACTIVITY_MEAL),
  ACT(8 * 60 + 0, "Service Work", ACTIVITY_WORK),
  ACT(11 * 60 + 0, "Lunch", ACTIVITY_MEAL),
  ACT(12 * 60 + 0, "Rest", ACTIVITY_REST),
  ACT(13 * 60 + 30, "Registration", ACTIVITY_WORK),
  ACT(16 * 60 + 0, "Prep", ACTIVITY_WORK),
  ACT(18 * 60 + 0, "Dinner", ACTIVITY_MEAL),
  ACT(19 * 60 + 30, "Orientation", ACTIVITY_INFO),
  ACT(22 * 60 + 0, "Lights out", ACTIVITY_SLEEP),
};

static const Activity k_server_day_one[] = {
  ACT(4 * 60 + 30, "Wake up", ACTIVITY_OTHER),
  ACT_MED(5 * 60 + 0, "Meditation"),
  ACT(6 * 60 + 30, "Breakfast", ACTIVITY_MEAL),
  ACT(7 * 60 + 30, "Kitchen", ACTIVITY_WORK),
  ACT(9 * 60 + 30, "Service Work", ACTIVITY_WORK),
  ACT(11 * 60 + 0, "Lunch", ACTIVITY_MEAL),
  ACT(12 * 60 + 0, "Rest", ACTIVITY_REST),
  ACT(13 * 60 + 0, "Service Work", ACTIVITY_WORK),
  ACT(17 * 60 + 0, "Tea", ACTIVITY_MEAL),
  ACT_MED(18 * 60 + 0, "Meditation"),
  ACT(19 * 60 + 0, "Discourse", ACTIVITY_DISCOURSE),
  ACT(20 * 60 + 15, "Service Work", ACTIVITY_WORK),
  ACT(22 * 60 + 0, "Lights out", ACTIVITY_SLEEP),
};

static const Activity k_server_day_four[] = {
  ACT(4 * 60 + 30, "Wake up", ACTIVITY_OTHER),
  ACT_MED(5 * 60 + 0, "Meditation"),
  ACT(6 * 60 + 30, "Breakfast", ACTIVITY_MEAL),
  ACT(7 * 60 + 30, "Service Work", ACTIVITY_WORK),
  ACT(10 * 60 + 0, "Meeting", ACTIVITY_INFO),
  ACT(11 * 60 + 0, "Lunch", ACTIVITY_MEAL),
  ACT(12 * 60 + 0, "Rest", ACTIVITY_REST),
  ACT(13 * 60 + 0, "Support", ACTIVITY_WORK),
  ACT(17 * 60 + 0, "Tea", ACTIVITY_MEAL),
  ACT_MED(18 * 60 + 0, "Meditation"),
  ACT(19 * 60 + 0, "Discourse", ACTIVITY_DISCOURSE),
  ACT(20 * 60 + 15, "Service Work", ACTIVITY_WORK),
  ACT(22 * 60 + 0, "Lights out", ACTIVITY_SLEEP),
};

static const Activity k_server_day_ten[] = {
  ACT(4 * 60 + 30, "Wake up", ACTIVITY_OTHER),
  ACT_MED(5 * 60 + 0, "Meditation"),
  ACT(6 * 60 + 30, "Breakfast", ACTIVITY_MEAL),
  ACT(7 * 60 + 30, "Service Work", ACTIVITY_WORK),
  ACT(10 * 60 + 0, "Meeting", ACTIVITY_INFO),
  ACT(11 * 60 + 0, "Lunch", ACTIVITY_MEAL),
  ACT(12 * 60 + 0, "Rest", ACTIVITY_REST),
  ACT(13 * 60 + 30, "Support", ACTIVITY_WORK),
  ACT(17 * 60 + 0, "Dinner", ACTIVITY_MEAL),
  ACT_MED(18 * 60 + 0, "Meditation"),
  ACT(19 * 60 + 0, "Discourse", ACTIVITY_DISCOURSE),
  ACT(20 * 60 + 15, "Rest", ACTIVITY_REST),
  ACT(22 * 60 + 0, "Lights out", ACTIVITY_SLEEP),
};

static const Activity k_server_day_eleven[] = {
  ACT(5 * 60 + 0, "Wake up", ACTIVITY_OTHER),
  ACT_MED(5 * 60 + 30, "Meditation"),
  ACT(6 * 60 + 30, "Cleaning", ACTIVITY_WORK),
  ACT(7 * 60 + 30, "Breakfast", ACTIVITY_MEAL),
  ACT(8 * 60 + 30, "Pack", ACTIVITY_OTHER),
  ACT(10 * 60 + 0, "Meeting", ACTIVITY_INFO),
  ACT(11 * 60 + 0, "Lunch", ACTIVITY_MEAL),
  ACT(12 * 60 + 30, "Rest", ACTIVITY_REST),
  ACT(14 * 60 + 0, "Service Work", ACTIVITY_WORK),
  ACT(17 * 60 + 0, "Dinner", ACTIVITY_MEAL),
  ACT_METTA(18 * 60 + 0, "Metta"),
  ACT(20 * 60 + 0, "Lights out", ACTIVITY_SLEEP),
};

typedef struct {
  DayType day_type;
  const Activity *activities;
  size_t count;
} DayTypeTable;

static DaySchedule schedule_from_daytype(const DayTypeTable *table, size_t count, DayType day_type) {
  for (size_t i = 0; i < count; i++) {
    if (table[i].day_type == day_type) {
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

DayType schedule_get_day_type(CourseType course_type, int day) {
  if (course_type != COURSE_TYPE_TEN_DAY) {
    return DAYTYPE_ANAPANA;
  }

  if (day <= -1 || day >= 12) {
    return DAYTYPE_PRE_COURSE;
  }

  if (day == 0) {
    return DAYTYPE_ARRIVAL;
  }

  if (day >= 1 && day <= 3) {
    return DAYTYPE_ANAPANA;
  }

  if (day == 4) {
    return DAYTYPE_TRANSITION;
  }

  if (day >= 5 && day <= 9) {
    return DAYTYPE_VIPASSANA;
  }

  if (day == 10) {
    return DAYTYPE_METTA;
  }

  return DAYTYPE_DEPARTURE;
}

MeditationType schedule_meditation_for_day(DayType day_type) {
  switch (day_type) {
    case DAYTYPE_ANAPANA:
    case DAYTYPE_ARRIVAL:
    case DAYTYPE_PRE_COURSE:
      return MEDITATION_ANAPANA;
    case DAYTYPE_TRANSITION:
    case DAYTYPE_VIPASSANA:
      return MEDITATION_VIPASSANA;
    case DAYTYPE_METTA:
    case DAYTYPE_DEPARTURE:
      return MEDITATION_METTA;
    default:
      return MEDITATION_NONE;
  }
}

DaySchedule schedule_get_day(CourseType course_type, CourseRole role, int day) {
  DayType day_type = schedule_get_day_type(course_type, day);

  if (role == COURSE_ROLE_SERVER) {
    static const DayTypeTable k_server_table[] = {
      {DAYTYPE_PRE_COURSE, k_server_day_minus_one, ARRAY_LENGTH(k_server_day_minus_one)},
      {DAYTYPE_ARRIVAL, k_server_day_zero, ARRAY_LENGTH(k_server_day_zero)},
      {DAYTYPE_ANAPANA, k_server_day_one, ARRAY_LENGTH(k_server_day_one)},
      {DAYTYPE_TRANSITION, k_server_day_four, ARRAY_LENGTH(k_server_day_four)},
      {DAYTYPE_VIPASSANA, k_server_day_one, ARRAY_LENGTH(k_server_day_one)},
      {DAYTYPE_METTA, k_server_day_ten, ARRAY_LENGTH(k_server_day_ten)},
      {DAYTYPE_DEPARTURE, k_server_day_eleven, ARRAY_LENGTH(k_server_day_eleven)},
    };
    return schedule_from_daytype(k_server_table, ARRAY_LENGTH(k_server_table), day_type);
  }

  static const DayTypeTable k_student_table[] = {
    {DAYTYPE_PRE_COURSE, k_student_day_minus_one, ARRAY_LENGTH(k_student_day_minus_one)},
    {DAYTYPE_ARRIVAL, k_student_day_zero, ARRAY_LENGTH(k_student_day_zero)},
    {DAYTYPE_ANAPANA, k_student_day_one, ARRAY_LENGTH(k_student_day_one)},
    {DAYTYPE_TRANSITION, k_student_day_four, ARRAY_LENGTH(k_student_day_four)},
    {DAYTYPE_VIPASSANA, k_student_day_one, ARRAY_LENGTH(k_student_day_one)},
    {DAYTYPE_METTA, k_student_day_ten, ARRAY_LENGTH(k_student_day_ten)},
    {DAYTYPE_DEPARTURE, k_student_day_eleven, ARRAY_LENGTH(k_student_day_eleven)},
  };

  return schedule_from_daytype(k_student_table, ARRAY_LENGTH(k_student_table), day_type);
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

const char *schedule_meditation_label(MeditationType meditation) {
  switch (meditation) {
    case MEDITATION_ANAPANA:
      return "Anapana";
    case MEDITATION_VIPASSANA:
      return "Vipassana";
    case MEDITATION_METTA:
      return "Metta";
    default:
      return "";
  }
}
