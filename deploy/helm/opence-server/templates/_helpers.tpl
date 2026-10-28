{{/* The objects' name: fullnameOverride, else the release name if it contains the chart's name,
   else <release>-<chart>. */}}
{{- define "opence-server.fullname" -}}
{{- if .Values.fullnameOverride -}}
{{- .Values.fullnameOverride | trunc 63 | trimSuffix "-" -}}
{{- else -}}
{{- $name := default .Chart.Name .Values.nameOverride -}}
{{- if contains $name .Release.Name -}}
{{- .Release.Name | trunc 63 | trimSuffix "-" -}}
{{- else -}}
{{- printf "%s-%s" .Release.Name $name | trunc 63 | trimSuffix "-" -}}
{{- end -}}
{{- end -}}
{{- end }}

{{- define "opence-server.name" -}}
{{- default .Chart.Name .Values.nameOverride | trunc 63 | trimSuffix "-" -}}
{{- end }}

{{- define "opence-server.labels" -}}
{{ include "opence-server.selectorLabels" . }}
app.kubernetes.io/managed-by: {{ .Release.Service }}
helm.sh/chart: {{ printf "%s-%s" .Chart.Name .Chart.Version | replace "+" "_" | trunc 63 | trimSuffix "-" }}
{{- end }}

{{- define "opence-server.selectorLabels" -}}
app.kubernetes.io/name: {{ include "opence-server.name" . }}
app.kubernetes.io/instance: {{ .Release.Name }}
{{- end }}

{{/* repository:tag, repository:tag@digest, or repository@digest; an empty tag is the appVersion. */}}
{{- define "opence-server.image" -}}
{{- $image := .Values.image -}}
{{- $ref := $image.repository -}}
{{- $tag := toString ($image.tag | default .Chart.AppVersion) -}}
{{- if $image.digest }}{{ $tag = regexReplaceAll "@.*$" $tag "" }}{{ if hasPrefix "sha256:" $tag }}{{ $tag = "" }}{{ end }}{{ end -}}
{{- if hasPrefix "sha256:" $tag }}{{ $ref = printf "%s@%s" $ref $tag }}
{{- else if $tag }}{{ $ref = printf "%s:%s" $ref $tag }}{{ end -}}
{{- if and $image.digest (not (contains "@" $ref)) }}{{ $ref = printf "%s@%s" $ref $image.digest }}{{ end -}}
{{- $ref -}}
{{- end }}

{{- define "opence-server.stateClaim" -}}
{{- .Values.persistence.existingClaim | default (printf "%s-state" (include "opence-server.fullname" .)) -}}
{{- end }}

{{/* The chart's Secret: the web passwords it makes or is given, the join password, the publisher
   key. */}}
{{- define "opence-server.secretName" -}}
{{- printf "%s-secrets" (include "opence-server.fullname" .) -}}
{{- end }}

{{/* The Secret holding the web password: web.existingSecret, else the chart's. */}}
{{- define "opence-server.webSecret" -}}
{{- .Values.web.existingSecret | default (include "opence-server.secretName" .) -}}
{{- end }}

{{- define "opence-server.hasGameData" -}}
{{- with .Values.gameData -}}
{{- if or .existingClaim (and .nfs.server .nfs.path) .volume }}true{{ end -}}
{{- end -}}
{{- end }}

{{- define "opence-server.hasCustomMaps" -}}
{{- with .Values.customMaps -}}
{{- if or .existingClaim (and .nfs.server .nfs.path) .volume }}true{{ end -}}
{{- end -}}
{{- end }}

{{/* A volume source of gameData or customMaps (the dict), read-only. */}}
{{- define "opence-server.volumeSource" -}}
{{- if .existingClaim }}
persistentVolumeClaim:
  claimName: {{ .existingClaim }}
  readOnly: true
{{- else if and .nfs.server .nfs.path }}
nfs:
  server: {{ .nfs.server }}
  path: {{ .nfs.path }}
  readOnly: true
{{- else }}
{{- toYaml .volume }}
{{- end }}
{{- end }}
