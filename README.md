# Harpoon2

This is a very early and experimental prototype.

# Building the Project

```
cd build
cmake .. -DUSE_WEBSOCKETPP_FROM_GIT=1 -DUSE_UTFCPP_FROM_GIT=1
make -j4
```

# Running the Tool

From the project root call the binary:
```
./bin/harpoon2 --username myuser --password mypassword --channel harpoon
```
