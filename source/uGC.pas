unit uGC;

interface

implementation

uses
	Windows;

function NGetMem(Size: Integer): Pointer;
begin
  Result := VirtualAlloc(nil, Size, MEM_COMMIT or MEM_TOP_DOWN, PAGE_READWRITE);
end;

function NFreeMem(P: Pointer): Integer;
begin
  if VirtualFree(P, 0, MEM_RELEASE) then
    Result := 0
  else
    Result := 1;
end;

function NReallocMem(P: Pointer; Size: Integer): Pointer;
begin
  Result := NGetMem(Size);
  System.Move(P^, Result^, Size);
  NFreeMem(P);
end;

function NAllocMem(Size: Cardinal): Pointer;
begin
  Result := VirtualAlloc(nil, Size, MEM_COMMIT or MEM_TOP_DOWN, PAGE_READWRITE);
end;

procedure InitializaGc;
var
  MemOld: TMemoryManagerEx;
begin
  GetMemoryManager(MemOld);
  MemOld.GetMem := NGetMem;
  MemOld.FreeMem := NFreeMem;
  MemOld.ReallocMem := NReallocMem;
  MemOld.AllocMem := NAllocMem;
  SetMemoryManager(MemOld);
end;

initialization
  InitializaGc;

end.
