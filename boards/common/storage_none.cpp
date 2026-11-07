#include "storage_profile.h"
const BoardStorageProfile &xtouch_board_storage_profile() {
    static const BoardStorageProfile profile{};
    return profile;
}
