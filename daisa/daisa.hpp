#pragma once
#if defined _WIN32 || defined __CYGWIN__
  #ifdef BUILDING_DAISA
    #define DAISA_PUBLIC __declspec(dllexport)
  #else
    #define DAISA_PUBLIC __declspec(dllimport)
  #endif
#else
  #ifdef BUILDING_DAISA
      #define DAISA_PUBLIC __attribute__ ((visibility ("default")))
  #else
      #define DAISA_PUBLIC
  #endif
#endif

namespace daisa {

class DAISA_PUBLIC Daisa {

public:
  Daisa();
  int get_number() const;

private:

  int number;

};

}

