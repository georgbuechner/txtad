#ifndef SHARED_UTILS_DEFINES_H
#define SHARED_UTILS_DEFINES_H

enum UseCtx {
  NO = 0,
  REGEX,
  Name,
  Name_STARTS_WITH,
  Name_FUZZY,
  Name_FUZZY_OR_STARTS_WITH,
};

#endif
