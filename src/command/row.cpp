#include <row.h>
#include <string>

void UserInfo::serialize(void* destination) {
    // serialize id
    int * id_ptr = (int *) destination;
    *id_ptr = id;

    // serialize username
    char * char_ptr = (char *) (id_ptr + 1);
    for (int i=0; i < COL_USERNAME_SIZE; ++i) {
        *char_ptr = username[i];
        char_ptr += 1;
    }

    // serialize email
    for (int i=0; i < COL_EMAIL_SIZE; ++i) {
        *char_ptr = email[i];
        char_ptr += 1;
    }
}

void UserInfo::deserialize(void *destination) {
    // serialize id
    int * id_ptr = (int *) destination;
    id = *id_ptr;

    // deserialize username
    char * char_ptr = (char *) (id_ptr + 1);
    for (int i=0; i < COL_USERNAME_SIZE; ++i) {
        username[i] = *char_ptr;
        char_ptr += 1;
    }

    // deserialize email
    for (int i=0; i < COL_EMAIL_SIZE; ++i) {
        email[i] = *char_ptr;
        char_ptr += 1;
    }
}

std::string UserInfo::to_string() {
    return std::to_string(id) + ',' +
        std::string(username) + ',' +
        std::string(email);
}