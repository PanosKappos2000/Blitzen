/*
    Including structs to improve my older versions
*/

struct IndirectDrawCommand {
    uint indexCount;
    uint instanceCount;
    uint firstIndex;
    uint vertexOffset;

    uint firstInstance;
    uint objectId;     // Metadata

    uint padding0;
    uint padding1;     // Pad to 32 bytes (or 48 if needed)
};