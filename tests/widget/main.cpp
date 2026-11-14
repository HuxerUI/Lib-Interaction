#include "../support/test.h"

void CheckValidation();

#ifdef INTERACTION_HAS_UI_TESTING
void CheckInteractionContracts();
void CheckGestureContracts();
#endif

int main() { return Run([] {
  CheckValidation();
#ifdef INTERACTION_HAS_UI_TESTING
  CheckInteractionContracts();
  CheckGestureContracts();
#endif
}); }
