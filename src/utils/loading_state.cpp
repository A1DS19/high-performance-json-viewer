#include "utils/loading_state.hpp"

LoadingState &get_loading_state() {
  static LoadingState state;
  return state;
}
