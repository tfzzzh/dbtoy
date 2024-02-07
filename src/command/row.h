#pragma once
#include <string>
#include <string.h>

/**
 * @brief interface for row of the database
 * 
 */
class Row {
public:
    /**
     * @brief serialize content of the row to destination
     * 
     * @param destination pointer to a storage 
     */
    virtual void serialize(void* destination) = 0;

    /**
     * @brief deserialize content from the storage
     * 
     * @param destination pointer to storage
     */
    virtual void deserialize(void* destination) = 0;

    /**
     * @brief return number of bytes needed to store the row
     * 
     * @return int 
     */
    virtual int get_row_byte() {return 0;}

    /**
     * @brief display the content of row
     * 
     * @return std::string 
     */
    virtual std::string to_string() = 0;

    virtual ~Row() {};
};

const int COL_USERNAME_SIZE = 32;
const int COL_EMAIL_SIZE = 32;
class UserInfo : public Row {
public:
    UserInfo(
        int id = 0, 
        const char * user = nullptr, 
        const char * mail = nullptr
    ): id(id) {
        if (user != nullptr)
            strcpy(username, user);

        if (mail != nullptr)
            strcpy(email, mail);
    }

    virtual void serialize(void* destination) override;

    virtual void deserialize(void* destination) override;

    virtual int get_row_byte() override {
        // [id, username '\0', email '\0']
        return sizeof(id) + COL_USERNAME_SIZE + COL_EMAIL_SIZE;
    }

    virtual std::string to_string() override;

private:
    int id;
    char username[COL_USERNAME_SIZE];
    char email[COL_EMAIL_SIZE];
};