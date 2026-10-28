import type { Part, Project } from './project';

export function focusPath(project: Project, focusId: string): Part[] {
  const path: Part[] = [];
  let node = project.parts.find(part => part.id === focusId);
  while (node) {
    path.unshift(node);
    node = project.parts.find(part => part.id === node?.parentId);
  }
  return path;
}

// An empty selection walks up one level; a selected component becomes the scope.
export function nextFocus(project: Project, focusId: string, selectedId: string): string {
  const selected = project.parts.find(part => part.id === selectedId);
  if (selected) return selected.id;
  const current = project.parts.find(part => part.id === focusId);
  return current?.parentId ?? current?.id ?? project.parts.find(part => part.kind === 'rocket')!.id;
}
