mtdp
----

`mtdp` is a C library aimed at handling software multi-threaded [data pipelines](https://en.wikipedia.org/wiki/Pipeline_(computing)).

## Use case

Pipelines are used to increase the [throughput](https://en.wikipedia.org/wiki/Network_throughput) of a data processing while maintaining almost equal [latency](https://en.wikipedia.org/wiki/Latency_(engineering)) by performing in parallel what are commonly performed sequentially. It is best suited for the following two cases:
1. to speed up calculations performed on a fixed, finite datastream
2. to support an infinite datastream in a real-time process

## Compatibility and dependencies
The library is implemented in pure C11 without dependencies except a C11 compliant runtime. It is currently supported in *NIX and Windows (not yet tested). Easy porting can be performed in environments supporting the C11 `<threads.h>` header (some efforts shall be made in porting futexes and semaphores in those cases).

Targets in the development are the optimization of resources needed to handle the pipelines in both speed and memory footprint, keeping the library as small as possible. 
_Experimental_: the memory allocation strategy may be selected by the user at compile time using a static global storage instead of the heap (useful for example in MISRA C environments).

## High-level description

A pipeline is architecturally composed of a variable number of stages actively cooperating on a data stream and a set of pipes connecting the stages internally. Each pipe contains a variable number of buffers provided by the user that are cyclically moved in a FIFO to enable a synchronized data transfer from the previous stage to the next.

The stages are distinguished in source stage producing data, internal stages that both consume and produce data, and a sink stage that consumes data. The output of the previous stage is fed to the next one as its input.

## Usage
The library exposes an `mtdp_pipeline` class together with its own API. After retrieving an instance of it, configure it providing the following resources:
1. references to the payload functions that will called repeatedly by the stages
2. resizing the pipes selecting the number of buffers they are going to use
3. providing the buffers to use, in each pipe. These buffers may be of a different type or size on each pipe.

You can then enable it and start it to put it in active mode.

As long as the pipeline will be active, data is allowed to flow from a stage up the next. APIs let you stop and/or resume the data flow at will.

When finished, remember to deallocate the resources:
1. disable the pipeline first
2. retrieve and deallocate the buffers you provided
3. destroy the pipeline

For additional details, consult the documentation. If you have Doxygen installed you can build it building the `docs` CMake target.


### Concurrency
While multi-threaded, a pipeline was designed to be configured and controlled by a single thread. Actively configuring a pipeline from multiple threads is not a requirement for this library: this choice was made to reduce the footprint to a bare minimum, though passively waiting for pipeline activity is supported. If your software architecture needs to configure a pipeline from multiple threads you should provide your own synchronization mechanisms.

## Build instructions
To build the library, you will need a C11 compatible compiler and CMake. Run the following from the command line

```
mkdir -p build
cd build
cmake .. -DCMAKE_BUILD_TYPE:String=Release
cmake --build . 
```

On *NIX you can find two examples showing the usage of the library compiled in the build directory as `mtdp_infinite_datastream_example` and `mtdp_finite_datastream_example`.

CPack may be used to pack the library in a zst. Simply run from the build directory

```
cpack
```

after the build commands (examples will not be included in the package).

### Build configuration (experimental)
By default, the memory allocation strategy uses `malloc`/`free` to allocate and deallocate memory. Constrained environments not relying on the heap (e.g. MISRA C complaint embedded software) may configure the following compile-time settings to switch the allocation strategy to a static global storage that keeps the `malloc`/`free` interface. It is thread safe by default, but that can be disabled if you plan to use the pipelines from a single thread.
The following parameters may be additional set while configuring with CMake:  

|Parameter|Default Value|Description
|-----|----|----|
|`MTDP_STATIC_THREADSAFE`                       | true    |    |
|`MTDP_STATIC_THREADSAFE_LOCKFREE`              | true    |    |
|`MTDP_PIPELINE_STATIC_INSTANCES`               | 0       |    |
|`MTDP_PIPE_VECTOR_STATIC_SIZE`                 | 0       |    |
|`MTDP_STAGE_VECTOR_STATIC_SIZE`                | 0       |    |
|`MTDP_STAGE_IMPL_VECTOR_STATIC_SIZE`           | 0       |    |
|`MTDP_BUFFER_POOL_STATIC_SIZE`                 | 0       |    |
|`MTDP_BUFFER_FIFO_BLOCKS`                      | 0       |    |
|`MTDP_BUFFER_FIFO_BLOCK_VECTOR_STATIC_SIZE`    | 0       |    |
|`MTDP_BUFFER_FIFO_BLOCK_STATIC_INSTANCES`      | 0       |    |
|`MTDP_BUFFER_FIFO_SHIFT_FILLING_RATIO`         | 0.5     |    |
|`MTDP_BUFFER_FIFO_BLOCK_SIZE`                  | 16      |    |
|`MTDP_PIPELINE_CONSUMER_TIMEOUT_US`            | 100000  |    |
|`MTDP_STRICT_ISO_C`                            | false    |    |

## Known bugs
None at the moment, but if any are found please feel free to open an issue. Code quality tools TBI.