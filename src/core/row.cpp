#include <row.h>
#include <sstream>
#include <cassert>

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

// str if of the form
// 1 alice alice@333.com
void UserInfo::from_string(const std::string &str) {
    std::stringstream oin(str);
    int field1;
    std::string field2;
    std::string field3;

    oin >> field1 >> field2 >> field3;

    id = field1;

    assert(field2.size() < COL_USERNAME_SIZE);
    assert(field3.size() < COL_EMAIL_SIZE);
    
    strcpy(username, field2.c_str());
    strcpy(email, field3.c_str());
}