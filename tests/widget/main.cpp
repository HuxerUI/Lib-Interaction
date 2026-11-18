#include "../support/test.h"

void CheckValidation();
#ifdef INTERACTION_HAS_UI_TESTING
void CheckPressContracts();
void CheckHoverAndOverlayContracts();
void CheckGestureContracts();
void CheckReorderAndDragDropContracts();
void CheckScrollA11yAndVisualContracts();
#endif

int main() {
  return Run([] {
    CheckValidation();
#ifdef INTERACTION_HAS_UI_TESTING
    CheckPressContracts();
    CheckHoverAndOverlayContracts();
    CheckGestureContracts();
    CheckReorderAndDragDropContracts();
    CheckScrollA11yAndVisualContracts();
#endif
  });
}
