#!/usr/bin/env python3
import json
import sys

EXPECTED={"O30-01","O30-02","O30-03","O30-04","O30-05","O30-06"}
MASTER={"O30-01","O30-02","O30-03","O30-06"}
GAMEPLAY={"O30-04","O30-05"}

def validate(data):
    errors=[]
    rows=data.get("rows",[])
    ids=[row.get("id") for row in rows]
    if len(ids)!=6 or set(ids)!=EXPECTED:
        errors.append("expected exactly O30-01..O30-06")
    if len(ids)!=len(set(ids)):
        errors.append("duplicate id")
    for row in rows:
        ident=row.get("id")
        owner=row.get("runtime_owner")
        if ident in MASTER and owner!="integration-master-r01":
            errors.append(f"{ident}: wrong master owner")
        if ident in GAMEPLAY and owner!="gameplay-runtime-r01":
            errors.append(f"{ident}: wrong gameplay owner")
        if not row.get("research_authority") or not row.get("owner_scope") or not row.get("implementation_status"):
            errors.append(f"{ident}: incomplete assignment")
        status=str(row.get("implementation_status",""))
        if status=="CLOSED" or "HARDWARE_PASS" in status:
            errors.append(f"{ident}: ownership assignment cannot claim implementation closure")
    return errors

def main(path):
    data=json.load(open(path,"r",encoding="utf-8"))
    errors=validate(data)
    if errors:
        print("FAIL")
        for error in errors:
            print(" -",error)
        return 1
    print("PASS")
    return 0

if __name__=="__main__":
    if len(sys.argv)!=2:
        raise SystemExit("usage: validate_wp30_ownership_resolution.py manifest.json")
    raise SystemExit(main(sys.argv[1]))
