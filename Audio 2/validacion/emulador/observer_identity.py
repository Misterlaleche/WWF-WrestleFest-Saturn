"""Checked read-only observer adaptation for direct HWRAM music loads."""
def direct_load_identity(source):
    old="    out={}\n    for name,elf in audio_elfs.items():"
    new="    work,_=field(state,'WorkRAMH',1048576)\n"+old
    if source.count(old)!=1:
        raise ValueError('Review the upstream code-identity observer before adapting it')
    source=source.replace(old,new)
    old="            offset=ram+address if name=='SCSP.ELF' else vram+address-0x05c00000"
    new="""            if name=='SCSP.ELF':
                if address<0 or address+length>524288:raise ValueError('SCSP identity outside sound RAM')
                offset=ram+address
            elif 0x06000000<=address and address+length<=0x06100000:
                offset=work+address-0x06000000
            elif 0x05c00000<=address and address+length<=0x05c80000:
                offset=vram+address-0x05c00000
            else:raise ValueError('Music executable outside the declared RAM regions')"""
    if source.count(old)!=1:
        raise ValueError('Review target-memory selection before adapting the observer')
    return source.replace(old,new)
