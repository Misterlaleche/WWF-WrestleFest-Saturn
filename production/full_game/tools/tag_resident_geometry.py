"""Bounded expansion of the recovered secondary lease; codec unchanged."""
TILES=16384
BLOCKS=TILES//8
RESIDENT=17760+102400
INPUT=1024
WORKSPACE=53248+2048+102400+(INPUT-704)
# The expanded TAG-only LWRAM layout used by the no-cart profile owner.
# Keep the original default for existing build_session callers.
GEOMETRIES={
    'legacy16384': (TILES,BLOCKS,RESIDENT,WORKSPACE,INPUT,0x9ec00),
    'tag18432': (18432,2304,117696,158016,INPUT,0x9ac00),
}
def project_header(source,diagnostic_bytes=0):
    assert diagnostic_bytes in (0,16384)
    replacements={'TILES 14352u':f'TILES {TILES}u','BLOCKS 1794u':f'BLOCKS {BLOCKS}u',
        'WORKSPACE_BYTES 49152u':f'WORKSPACE_BYTES {WORKSPACE-diagnostic_bytes}u',
        'metadata_padding[12]':'metadata_padding[28]', 'resident[13664]':f'resident[{RESIDENT-diagnostic_bytes}]',
        'INPUT_BYTES 704u':f'INPUT_BYTES {INPUT}u',
        'tables)==23200u':'tables)==25248u', 'cd_scratch)==25824u':'cd_scratch)==27872u',
        'input)==30528u':'input)==32576u', 'output)==31232u':f'output)=={33280+INPUT-704}u',
        'resident)==35456u':f'resident)=={37504+INPUT-704}u', 'end_guard)==49120u':f'end_guard)=={WORKSPACE-diagnostic_bytes-32}u'}
    for before,after in replacements.items():
        assert source.count(before)==1,before
        source=source.replace(before,after)
    return source
