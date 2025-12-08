# FTest - A Functional Testing Framework in Zephyr

FTest is a framework for Zephyr's Native Simulator that can host multiple
devices communicating over a network. It features the following capabilities:

Add a new comment here!

- Full reproducibility
- Faster than realtime execution - 90 times the normal speed
- Communication over an emulated Ethernet network
- Control over GPIO and peripherals from the test environment

# Try it

To use it, build a docker dev environment first, and enter the built dev
container using
[vscode devcontainers extension](https://code.visualstudio.com/docs/devcontainers/containers).
Then inside, go to `runner` directory and run:

```bash
west build -p -b nrf5340/cpuapp/ns 
```

To run the test afterwards, run 

```bash
../build/zephyr/zephyr.exe 
```

You should see the output of the test.
