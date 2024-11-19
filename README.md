# Cinema 4D C++ API Examples 

Contains the official code examples for the Cinema 4D C++ API.

The provided code examples are identical to the ones shipped with the [Cinema 4D C++ SDK](https://developers.maxon.net/downloads/). See our [Cinema 4D C++ API Documentation](https://developers.maxon.net/docs/cpp) for written manuals and an API index.

To get started with the Cinema 4D C++ API, we recommend reading the [Getting Started: First Steps](https://developers.maxon.net/docs/cpp/2025_0_1/page_maxonapi_getting_started_introduction.html) manual. We also recommend visiting and registering at [developers.maxon.net](https://developers.maxon.net/) to be able to generate plugin identifiers and to participate in our [developer forum](https://developers.maxon.net/forum/).

## Content

>  :warning: This repository does neither contain the frameworks nor the build tools required to build the code examples. Please use our [Cinema 4D C++ SDK](https://developers.maxon.net/downloads/) for acquiring a complete build environment.

| Directory | Description |
| :- | :- |
| plugins/example.main | Provides the majority of examples using the Cinema API. This will be for most users the most important and only relevant example pool. |
| plugins/example.nodes | Provides examples for interacting with and implementing node systems of the Nodes API as represented by the Nodes Editor in Cinema 4D. Relevant for developers who want to support the Nodes API in their plugins. |
| plugins/example.assets | Provides examples for reading, creating, and implementing assets for the Asset Browser of Cinema 4D. |
| plugins/example.image | Provides examples for reading and writing image data with the Maxon Image API. Currently mostly focused on OCIO and color management. |
| plugins/example.migration_2024 | Provides examples for migrating plugins using legacy APIs to the Cinema 4D 2024.0.0 API. |
