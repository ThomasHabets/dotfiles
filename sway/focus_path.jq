def unwrap:
  .[1] as $path
  | .[0][]
  | if (.focused) then {path: $path, focused: .focused} else
      if (.nodes|length > 0) then [.nodes, $path + [.layout]] | unwrap
      else {} end
    end;
[[.]] | unwrap | select(.focused == true) | .path | map(.=="tabbed") | reverse | index(true)
