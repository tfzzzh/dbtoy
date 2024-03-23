This project is a simple implementation of database.   It supports the following function:
* Persist a table into disk
* Load table into memory
* Use B+ tree to perform queries
* Persist the B+ tree into disk

### Build
```
chmod +x ./build.sh
./build.sh
```
### Run Tests
```
cd ./build/test
ctest
```
### Run Queries
Open the database
```
cd ./build/src
./dbtoy
```
perform queries
```
db > insert 1 alice alice@google.com
db > insert 2 bob bob@yahoo.com
db > select
db > .exit
```