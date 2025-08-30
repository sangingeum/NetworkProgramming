if not exist "generated" mkdir generated
protoc -I ./idl --cpp_out=./generated ./idl/*.proto