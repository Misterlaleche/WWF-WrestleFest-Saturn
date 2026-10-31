"""Validate coherent active-scene publications; never count frozen DONE samples."""
class ClockEvidence:
    def __init__(self):
        self.count=0
        self.max_pending=0
        self.last_sequence=None
    def add(self,s):
        if not s['boundary_stable'] or s['wf_logic_sequence']&1 or s['wf_state'] not in (1,3,4):
            return
        fields,ticks,pending=(s['wf_logic_fields'],s['wf_logic_ticks'],s['wf_logic_pending'])
        if ticks+pending != fields*37609//39168:
            raise ValueError('native clock lost/added debt: '+repr((fields,ticks,pending)))
        if fields and s['wf_logic_sequence']!=self.last_sequence:
            self.count+=1
            self.max_pending=max(self.max_pending,pending)
            self.last_sequence=s['wf_logic_sequence']
