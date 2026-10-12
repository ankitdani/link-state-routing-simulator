# Stage 1: compile the simulator and run the tests.
# If any test fails, the image build stops here.
FROM gcc:14 AS build
WORKDIR /src
COPY src/ src/
COPY tests/ tests/
RUN g++ -std=c++17 -Wall -O2 -static -o router_sim src/main.cpp src/Network.cpp \
 && g++ -std=c++17 -Wall -O2 -o run_tests tests/test_network.cpp src/Network.cpp \
 && ./run_tests

# Stage 2: a small image with just the program and the example files.
FROM debian:bookworm-slim
WORKDIR /app
COPY --from=build /src/router_sim /usr/local/bin/router_sim
COPY examples/ examples/
ENTRYPOINT ["router_sim"]
CMD ["examples/network.txt"]
